// SPDX-License-Identifier: LGPL-2.1-or-later
// Parts derived from chibicc by Rui Ueyama.
//
// Token-stream preprocessor. The lexer is the single scanner: every source
// file is lexed exactly once (lex_one in pp-mode) and the preprocessor
// consumes and produces Token*. Macro expansion (arguments, # stringize,
// ## paste, __VA_ARGS__/__VA_OPT__, blue paint) operates on token lists;
// macro bodies are lexed once at define time. There is no text round-trip
// between preprocessing and parsing.
#include "rcc.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef _WIN32
#define PATHSEP "\\"
#else
#define PATHSEP "/"
#endif

typedef struct Macro Macro;
typedef struct OnceFile OnceFile;
typedef struct CondIncl CondIncl;

struct Macro {
    Macro *next;
    Macro *hash_next;
    uint32_t hash;
    char *name;
    bool is_function;
    bool is_variadic;
    bool is_gnu_variadic;
    bool disabled;
    char **params;
    int param_len;
    Token *body;
    unsigned hh_mask;
};

#define MACRO_HT_SIZE 2048
static Macro *macro_htab[MACRO_HT_SIZE];

static char *kw_line;
static char *kw_file;
static char *kw_counter;
static char *kw_function;
static char *kw_func;
static char *kw_pretty_function;
static char *kw_has_include;
static char *kw_has_include_next;
static char *kw_has_c_attribute;
static char *kw_va_args;
static char *kw_va_opt;
static char *kw_defined;
static char *kw_true;
static char *kw_false;

static char *dn_define, *dn_undef, *dn_include, *dn_line, *dn_error, *dn_warning;
static char *dn_if, *dn_ifdef, *dn_ifndef, *dn_elif, *dn_elifdef, *dn_elifndef;
static char *dn_else, *dn_endif, *dn_pragma;

// Forward declaration: a conditional-compilation directive (#ifdef/#else/
// #endif/...) is a GNU extension when it appears in the middle of a
// function-like macro's argument list (e.g. the kernel's struct_group()
// wrapping members behind #ifdef CONFIG_FOO). The macro-argument collector
// needs to process it in place rather than giving up on the whole call.
static void do_directive(void);

static uint32_t macro_hash(const char *name) {
    uint64_t v = (uint64_t)(uintptr_t)name;
    v *= 0x9E3779B97F4A7C15ull;
    return (uint32_t)(v >> 45) & (MACRO_HT_SIZE - 1);
}

static void macro_ht_add(Macro *m) {
    uint32_t h = macro_hash(m->name);
    m->hash = h;
    m->hash_next = macro_htab[h];
    macro_htab[h] = m;
}

static void macro_ht_remove(const char *name) {
    char *iname = str_intern(name, strlen(name));
    uint32_t h = macro_hash(iname);
    for (Macro **p = &macro_htab[h]; *p; p = &(*p)->hash_next)
        if ((*p)->name == iname) {
            *p = (*p)->hash_next;
            return;
        }
}

struct OnceFile {
    OnceFile *next;
    char *path;
};
struct CondIncl {
    CondIncl *next;
    bool parent_active, active, branch_taken;
};

// Dependency file tracking (for -Wp,-MMD)
typedef struct DepEntry DepEntry;
struct DepEntry {
    DepEntry *next;
    char *path;
};
static DepEntry *dep_files;
static void dep_add(const char *path) {
    for (DepEntry *d = dep_files; d; d = d->next)
        if (!strcmp(d->path, path)) return;
    DepEntry *d = arena_alloc(sizeof(DepEntry));
    d->path = (char *)path;
    d->next = dep_files;
    dep_files = d;
}

// Pre-include files (-include <file>)
static const char *preinclude_list[64];
static int nb_preinclude = 0;
void add_preinclude(const char *path) {
    if (nb_preinclude < 64)
        preinclude_list[nb_preinclude++] = path;
}

// Macro prefix map for diagnostics
void add_prefix_map(const char *old, const char *new_str) {
    opt_prefix_map_old = old;
    opt_prefix_map_new = new_str;
}

typedef struct MacroStack MacroStack;
struct MacroStack {
    MacroStack *next;
    char *name;
    bool is_function, is_variadic, is_gnu_variadic;
    char **params;
    int param_len;
    Token *body;
    unsigned hh_mask;
};

static Macro *macros;
static OnceFile *once_files;
static int pp_counter;
static int pp_cur_line;
static char *pp_cur_file;
static Macro *cmdline_macros;
static Macro *saved_macros;
static MacroStack *macro_stack;
static const char *user_include_paths[64];
static int nb_user_include_paths;
static bool macros_inited;

void add_include_path(const char *path) {
    if (nb_user_include_paths < 64)
        user_include_paths[nb_user_include_paths++] = str_intern(path, strlen(path));
}

static void clear_macros(void) {
    macros = saved_macros ? saved_macros : cmdline_macros;
    macro_stack = NULL;
    memset(macro_htab, 0, sizeof(macro_htab));
    for (Macro *m = macros; m; m = m->next) macro_ht_add(m);
}

int pack_align;
int pack_align_stack[16];
int pack_align_idx;
bool fenv_access;

void rcc_reset_state(void) {
    cmdline_macros = NULL;
    saved_macros = NULL;
    macros = NULL;
    macro_stack = NULL;
    memset(macro_htab, 0, sizeof(macro_htab));
    nb_user_include_paths = 0;
    macros_inited = false;
    pp_counter = 0;
    once_files = NULL;
    pack_align = 0;
    pack_align_idx = 0;
}

static void pp_warn(char *filename, unsigned line_no, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s:%u: warning: ", filename, line_no);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

static void pp_check_ident(char *name, int len, char *filename, unsigned line_no) {
    if (opt_Wno_homoglyph) return;
    const char *w = u8ident_check_ident_align16(name, len);
    if (w) pp_warn(filename, line_no, "%s", w);
}

static char *pp_strndup(const char *p, int len) {
    char *s = arena_alloc(len + 1);
    memcpy(s, p, len);
    s[len] = '\0';
    return s;
}

#define ptok(t, op) ((t) && (t)->kind == TK_PUNCT && (t)->len == (int)(sizeof(op)-1) && !memcmp((t)->ptr, op, sizeof(op)-1))

static Token *syn_punct(char *spelling, Token *site);
static Token *syn_ident(char *name, Token *site);
static Token *syn_num(int64_t val, Token *site);
static Token *syn_str(char *content, int clen, Token *site);

static Token *copy_token(Token *t) {
    Token *n = arena_alloc(sizeof(Token));
    *n = *t;
    n->next = NULL;
    return n;
}

static char *tok_spelling(Token *t, int *len) {
    if (t->kind == TK_STR && t->val > 0) {
        *len = (int)t->val;
        return t->ptr;
    }
    *len = t->len;
    return t->ptr;
}

static char *str_raw_contents(Token *t) {
    int pfx = t->string_literal_prefix ? (t->string_literal_prefix == '8' ? 2 : 1) : 0;
    int slen;
    char *sp = tok_spelling(t, &slen);
    return pp_strndup(sp + pfx + 1, slen - pfx - 2);
}

// Include-level stack
typedef struct PPLvl PPLvl;
struct PPLvl {
    PPLvl *next;
    char *p;
    int reported_line, line_idx;
    int *counts;
    char *buf;
    char *filename, *fpath;
    CondIncl *conds;
    bool bol, dead_in_comment, dead_in_string;
};

static PPLvl *lvl;
static int inc_depth;
static Token mark_eof = {.kind = TK_EOF};
static Token mark_directive = {.kind = TK_EOF};

static bool pp_active(void) { return !lvl->conds || lvl->conds->active; }
static void advance_line(void) {
    lvl->reported_line += lvl->counts ? lvl->counts[lvl->line_idx] : 1;
    lvl->line_idx++;
}

static Token *syn_punct(char *spelling, Token *site) {
    Token *t = arena_alloc(sizeof(Token));
    t->kind = TK_PUNCT;
    t->kw = ID_NONE;
    t->ptr = spelling;
    t->len = strlen(spelling);
    if (site) {
        t->filename = site->filename;
        t->lineno = site->lineno;
    } else if (lvl) {
        t->filename = lvl->filename;
        t->lineno = lvl->reported_line;
    }
    return t;
}
static Token *syn_ident(char *name, Token *site) {
    Token *t = arena_alloc(sizeof(Token));
    t->kind = TK_IDENT;
    t->kw = ID_NONE;
    int kw = keyword_id(name, strlen(name), NULL);
    if (kw != ID_NONE) {
        t->kw = kw;
        t->name = kw_canon[kw];
    } else
        t->name = str_intern(name, strlen(name));
    t->ptr = t->name;
    t->len = strlen(t->name);
    if (site) {
        t->filename = site->filename;
        t->lineno = site->lineno;
    } else if (lvl) {
        t->filename = lvl->filename;
        t->lineno = lvl->reported_line;
    }
    return t;
}
static Token *syn_num(int64_t val, Token *site) {
    char *s = format("%lld", (long long)val);
    Token *t = arena_alloc(sizeof(Token));
    t->kind = TK_NUM;
    t->kw = ID_NONE;
    t->val = val;
    t->ptr = s;
    t->len = strlen(s);
    if (site) {
        t->filename = site->filename;
        t->lineno = site->lineno;
    } else if (lvl) {
        t->filename = lvl->filename;
        t->lineno = lvl->reported_line;
    }
    return t;
}
static Token *syn_str(char *content, int clen, Token *site) {
    int cap = clen * 2 + 3;
    char *sp = arena_alloc(cap);
    int n = 0;
    sp[n++] = '"';
    for (int i = 0; i < clen; i++) {
        if (content[i] == '"' || content[i] == '\\') sp[n++] = '\\';
        sp[n++] = content[i];
    }
    sp[n++] = '"';
    Token *t = arena_alloc(sizeof(Token));
    t->kind = TK_STR;
    t->kw = ID_NONE;
    t->str = str_intern(content, clen);
    t->len = clen;
    t->ptr = sp;
    t->val = n;
    if (site) {
        t->filename = site->filename;
        t->lineno = site->lineno;
    } else if (lvl) {
        t->filename = lvl->filename;
        t->lineno = lvl->reported_line;
    }
    return t;
}
static Token *new_pp_token(TokenKind kind, Token *site) {
    Token *t = arena_alloc(sizeof(Token));
    t->kind = kind;
    t->kw = ID_NONE;
    if (site) {
        t->filename = site->filename;
        t->lineno = site->lineno;
    } else if (lvl) {
        t->filename = lvl->filename;
        t->lineno = lvl->reported_line;
    }
    return t;
}

// Macro table
static Macro *find_macro_interned(char *iname) {
    uint32_t h = macro_hash(iname);
    for (Macro *m = macro_htab[h]; m; m = m->hash_next)
        if (m->hash == h && m->name == iname) return m;
    return NULL;
}
static Macro *find_macro(char *name) { return find_macro_interned(str_intern(name, strlen(name))); }

static void push_macro(char *name) {
    name = str_intern(name, strlen(name));
    MacroStack *ms = arena_alloc(sizeof(MacroStack));
    ms->name = name;
    Macro *m = find_macro_interned(name);
    if (m) {
        ms->is_function = m->is_function;
        ms->is_variadic = m->is_variadic;
        ms->is_gnu_variadic = m->is_gnu_variadic;
        ms->param_len = m->param_len;
        ms->params = m->params;
        ms->body = m->body;
        ms->hh_mask = m->hh_mask;
    } else
        ms->param_len = -1;
    ms->next = macro_stack;
    macro_stack = ms;
}
static void pop_macro(char *name) {
    name = str_intern(name, strlen(name));
    MacroStack *ms = NULL;
    for (MacroStack **p = &macro_stack; *p; p = &(*p)->next)
        if ((*p)->name == name) {
            ms = *p;
            *p = (*p)->next;
            break;
        }
    if (!ms) return;
    if (ms->param_len < 0) {
        macro_ht_remove(name);
        Macro **pm = &macros;
        while (*pm) {
            if ((*pm)->name == name) {
                *pm = (*pm)->next;
                break;
            }
            pm = &(*pm)->next;
        }
    } else {
        Macro *m = find_macro(name);
        if (!m) {
            m = arena_alloc(sizeof(Macro));
            m->name = name;
            m->next = macros;
            macros = m;
            macro_ht_add(m);
        }
        m->is_function = ms->is_function;
        m->is_variadic = ms->is_variadic;
        m->is_gnu_variadic = ms->is_gnu_variadic;
        m->param_len = ms->param_len;
        m->params = ms->params;
        m->body = ms->body;
        m->hh_mask = ms->hh_mask;
    }
}
static int find_param_index(Macro *m, char *name) {
    for (int i = 0; i < m->param_len; i++)
        if (m->params[i] == name) return i;
    return -1;
}
static int va_slot(Macro *m) { return m->is_gnu_variadic && m->param_len > 0 ? m->param_len - 1 : m->param_len; }
static int param_or_va(Macro *m, char *name) {
    if (m->is_variadic && name == kw_va_args) return va_slot(m);
    return find_param_index(m, name);
}
static unsigned compute_hh_mask(Macro *m, Token *body) {
    unsigned mask = 0;
    Token *prev = NULL;
    for (Token *b = body; b && b->kind != TK_EOF; b = b->next) {
        if (b->kind == TK_IDENT) {
            int idx = param_or_va(m, b->name);
            if (idx >= 0) {
                Token *n1 = b->next;
                int is_hashhash = n1 && n1->kind == TK_PUNCT &&
                    n1->len == 2 && n1->ptr[0] == '#' && n1->ptr[1] == '#';
                int was_hashhash = prev && prev->kind == TK_PUNCT &&
                    prev->len == 2 && prev->ptr[0] == '#' && prev->ptr[1] == '#';
                if (is_hashhash) mask |= 1u << idx;
                if (was_hashhash) mask |= 1u << idx;
            }
        }
        prev = b;
    }
    return mask;
}
static void define_macro_tok(char *name, bool is_function, char **params, int param_len,
                             Token *body, bool is_variadic, bool is_gnu_variadic) {
    Macro *m = find_macro_interned(name);
    if (!m) {
        m = arena_alloc(sizeof(Macro));
        m->name = name;
        m->next = macros;
        macros = m;
        macro_ht_add(m);
    }
    m->is_function = is_function;
    m->is_variadic = is_variadic;
    m->is_gnu_variadic = is_gnu_variadic;
    m->disabled = false;
    m->params = params;
    m->param_len = param_len;
    m->body = body;
    m->hh_mask = compute_hh_mask(m, body);
}
static Token *lex_body_string(char *body, char *filename, int lineno) {
    char *save_input = current_input, *save_dbg = current_debug_filename;
    bool save_mode = lex_pp_mode;
    current_input = body;
    current_debug_filename = filename;
    lex_pp_mode = false;
    char *p = body;
    int ln = lineno;
    Token head = {};
    Token *tail = &head;
    for (Token *t; (t = lex_one(&p, &ln)) != NULL;) tail = tail->next = t;
    tail->next = NULL;
    current_input = save_input;
    current_debug_filename = save_dbg;
    lex_pp_mode = save_mode;
    return head.next;
}
static void define_macro(char *name, bool is_function, char **params, int param_len, char *body) {
    name = str_intern(name, strlen(name));
    char **pc = NULL;
    if (param_len > 0) {
        pc = arena_alloc(sizeof(char *) * param_len);
        for (int i = 0; i < param_len; i++) pc[i] = str_intern(params[i], strlen(params[i]));
    }
    Token *btoks = lex_body_string(body, "<builtin>", 1);
    define_macro_tok(name, is_function, pc, param_len, btoks, false, false);
}
void add_define(char *def) {
    char *eq = strchr(def, '='), *name, *body;
    if (eq) {
        name = str_intern(def, eq - def);
        body = pp_strndup(eq + 1, strlen(eq));
    } else {
        name = str_intern(def, strlen(def));
        body = pp_strndup("1", 1);
    }
    Macro *m = arena_alloc(sizeof(Macro));
    m->name = name;
    m->is_function = false;
    m->params = NULL;
    m->param_len = 0;
    m->body = lex_body_string(body, "<command line>", 1);
    m->hh_mask = 0;
    m->next = cmdline_macros;
    cmdline_macros = m;
}
void add_undef(char *name) {
    name = str_intern(name, strlen(name));
    macro_ht_remove(name);
    Macro **prev = &macros;
    for (Macro *m = macros; m; prev = &m->next, m = m->next)
        if (m->name == name) {
            *prev = m->next;
            return;
        }
}

// Remove a single #define previously added via add_define(), by name, from
// the persistent cmdline_macros seed list itself (add_undef() only touches
// the transient per-file `macros` table preprocess() rebuilds from that
// seed at the start of each call, so it can't undo a cmdline_macros entry
// for files processed *afterward*). Used to scope a synthetic compiler-
// internal macro (__ASSEMBLER__, predefined only while processing one
// particular .S/.s input, mirroring what every other C preprocessor does
// automatically for assembly-with-cpp mode) to just that one file in a
// multi-file compile, without touching any real -D the user passed.
void remove_cmdline_define(const char *name) {
    char *iname = str_intern(name, strlen(name));
    Macro **prev = &cmdline_macros;
    for (Macro *m = cmdline_macros; m; prev = &m->next, m = m->next)
        if (m->name == iname) {
            *prev = m->next;
            return;
        }
}
static bool is_once_file(char *path) {
    for (OnceFile *f = once_files; f; f = f->next)
        if (f->path == path) return true;
    return false;
}
static void mark_once_file(char *path) {
    if (is_once_file(path)) return;
    OnceFile *f = arena_alloc(sizeof(OnceFile));
    f->path = path;
    f->next = once_files;
    once_files = f;
}

// Path & file utilities
#include "sysinc_paths.h"
static char *path_dirname(char *path);
static char *path_join(const char *dir, const char *file);
static bool file_exists(const char *path);
static char *canonical_path(char *path);
static char *full_path(char *path);

static char *resolve_include(char *curr_file, char *spec, bool is_angle) {
    char *path;
    if (!is_angle) {
        char *dir = path_dirname(curr_file);
        path = path_join(dir, spec);
        if (file_exists(path)) return canonical_path(path);
    }
#ifndef RCC_INCDIR
#define RCC_INCDIR "include"
#endif
    path = path_join(RCC_INCDIR, spec);
    if (file_exists(path)) return canonical_path(path);
    if (strcmp(RCC_INCDIR, "include") != 0) {
        path = path_join("include", spec);
        if (file_exists(path)) return canonical_path(path);
    }
    for (int i = 0; i < nb_user_include_paths; i++) {
        path = path_join(user_include_paths[i], spec);
        if (file_exists(path)) return canonical_path(path);
    }
    if (!opt_nostdinc) {
        for (int i = 0; sys_include_paths[i]; i++) {
            path = path_join(sys_include_paths[i], spec);
            if (file_exists(path)) return canonical_path(path);
        }
    }
    if (file_exists(spec)) return canonical_path(spec);
    return NULL;
}

typedef struct {
    char *text;
    int *line_counts;
} SplicedInput;
static SplicedInput splice_lines_with_counts(char *input) {
    int len = strlen(input);
    char *buf = arena_alloc(len + 1);
    int *counts = arena_alloc(sizeof(int) * (len + 1));
    int j = 0, line_idx = 0, count = 1;
    for (int i = 0; i < len; i++) {
        if (input[i] == '\\' && input[i + 1] == '\n') {
            i++;
            count++;
        } else {
            buf[j++] = input[i];
            if (input[i] == '\n') {
                counts[line_idx++] = count;
                count = 1;
            }
        }
    }
    if (count > 1 || j == 0 || buf[j - 1] != '\n') counts[line_idx++] = count;
    buf[j] = '\0';
    return (SplicedInput){buf, counts};
}
static char *path_dirname(char *path) {
    char *last = path;
    for (char *p = path; *p; p++)
#ifdef _WIN32
        if (*p == '/' || *p == '\\')
#else
        if (*p == '/')
#endif
            last = p + 1;
    return pp_strndup(path, last - path);
}
char *path_basename(char *path) {
    char *last = path;
    for (char *p = path; *p; p++)
#ifdef _WIN32
        if (*p == '/' || *p == '\\')
#else
        if (*p == '/')
#endif
            last = p + 1;
    return last;
}
static char *path_join(const char *dir, const char *file) {
    if (!*dir) return str_intern(file, strlen(file));
#ifdef _WIN32
    return format("%s%s%s", dir, (dir[strlen(dir) - 1] == '/' || dir[strlen(dir) - 1] == '\\') ? "" : PATHSEP, file);
#else
    return format("%s%s%s", dir, (dir[strlen(dir) - 1] == '/') ? "" : PATHSEP, file);
#endif
}
static bool file_exists(const char *path) {
    FILE *fp = fopen(path, "r");
    if (fp) {
        fclose(fp);
        return true;
    }
    return false;
}
static char *full_path(char *path) {
    char full[4096];
#ifdef _WIN32
    if (_fullpath(full, path, sizeof(full))) return str_intern(full, strlen(full));
#else
    if (realpath(path, full)) return str_intern(full, strlen(full));
#endif
    return str_intern(path, strlen(path));
}
static char *canonical_path(char *path) {
    if (!path || !*path) return str_intern(path, path ? (int)strlen(path) : 0);
    char buf[4096];
    int len = strlen(path);
    if (len >= (int)sizeof(buf)) return str_intern(path, len);
    memcpy(buf, path, len + 1);
#ifdef _WIN32
    for (int i = 0; i < len; i++)
        if (buf[i] == '\\') buf[i] = '/';
#endif
    char *comps[256];
    int comp_lens[256], ncomp = 0;
    char *p = buf;
#ifdef _WIN32
    bool absolute = false;
    if (len >= 2 && ((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z')) && p[1] == ':') {
        absolute = true;
        p += 2;
    } else if (*p == '/') {
        absolute = true;
        p++;
    }
    while (*p == '/') p++;
#else
    bool absolute = (*p == '/');
    if (absolute) p++;
#endif
    while (*p) {
        char *start = p;
        while (*p && *p != '/') p++;
        int clen = p - start;
        while (*p == '/') p++;
        if (clen == 1 && start[0] == '.') continue;
        if (clen == 2 && start[0] == '.' && start[1] == '.') {
            if (ncomp > 0 && !(comp_lens[ncomp - 1] == 2 && comps[ncomp - 1][0] == '.' && comps[ncomp - 1][1] == '.')) ncomp--;
            else if (!absolute) {
                comps[ncomp] = start;
                comp_lens[ncomp] = clen;
                ncomp++;
            }
        } else {
            comps[ncomp] = start;
            comp_lens[ncomp] = clen;
            ncomp++;
        }
    }
    char out[4096];
    int dst = 0;
    if (absolute) {
#ifdef _WIN32
        if (buf[0] == '/') out[dst++] = '/';
        else {
            out[dst++] = buf[0];
            out[dst++] = ':';
            out[dst++] = '/';
        }
#else
        out[dst++] = '/';
#endif
    }
    for (int i = 0; i < ncomp; i++) {
        if (i > 0) out[dst++] = '/';
        memcpy(out + dst, comps[i], comp_lens[i]);
        dst += comp_lens[i];
    }
    if (dst == 0) out[dst++] = '.';
    out[dst] = '\0';
    return str_intern(out, dst);
}
static char *read_pp_file(char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    int cap = 10 * 1024 * 1024;
    char *buf = arena_alloc(cap);
    int size = fread(buf, 1, cap - 2, fp);
    fclose(fp);
    if (size == 0 || buf[size - 1] != '\n') buf[size++] = '\n';
    buf[size] = '\0';
    return buf;
}

// Raw token stream
static void push_level(char *display, char *fpath, char *contents) {
    if (++inc_depth > 245) error("Include depth exceeded in file: %s", fpath);
    SplicedInput sp = splice_lines_with_counts(contents);
    register_source_buffer(sp.text, sp.text + strlen(sp.text));
    PPLvl *l = arena_alloc(sizeof(PPLvl));
    l->p = sp.text;
    l->reported_line = 1;
    l->line_idx = 0;
    l->counts = sp.line_counts;
    l->buf = sp.text;
    l->filename = display;
    l->fpath = fpath;
    l->bol = true;
    l->next = lvl;
    lvl = l;
    current_input = l->buf;
    current_debug_filename = l->filename;
}
static bool pop_level(void) {
    if (!lvl->next) return false;
    inc_depth--;
    lvl = lvl->next;
    current_input = lvl->buf;
    current_debug_filename = lvl->filename;
    return true;
}
static void skip_bol_space(void) {
    char *p = lvl->p;
    for (;;) {
        if (*p == ' ' || *p == '\t') {
            p++;
            continue;
        }
        if (p[0] == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/')) {
                if (*p == '\n') {
                    lvl->p = p;
                    advance_line();
                }
                p++;
            }
            if (*p) p += 2;
            continue;
        }
        break;
    }
    lvl->p = p;
}
static void skip_dead_line(void) {
    char *p = lvl->p;
    while (*p && *p != '\n') {
        if (lvl->dead_in_comment) {
            if (p[0] == '*' && p[1] == '/') {
                p += 2;
                lvl->dead_in_comment = false;
            } else
                p++;
        } else if (lvl->dead_in_string) {
            if (*p == '\\' && p[1]) p += 2;
            else {
                if (*p == '"') lvl->dead_in_string = false;
                p++;
            }
        } else if (p[0] == '/' && p[1] == '*') {
            lvl->dead_in_comment = true;
            p += 2;
        } else if (p[0] == '/' && p[1] == '/') {
            while (*p && *p != '\n') p++;
        } else if (*p == '"') {
            lvl->dead_in_string = true;
            p++;
        } else
            p++;
    }
    lvl->dead_in_string = false;
    if (*p == '\n') p++;
    lvl->p = p;
    advance_line();
}
static Token *pp_next_raw(void) {
    for (;;) {
        if (lvl->bol) {
            skip_bol_space();
            char *p = lvl->p;
            if (*p == '\0') {
                if (!pop_level()) return &mark_eof;
                continue;
            }
            if (!pp_active()) {
                if (lvl->dead_in_comment || lvl->dead_in_string) {
                    skip_dead_line();
                    continue;
                }
                if (*p == '#') return &mark_directive;
                if (*p == '\n') {
                    lvl->p = p + 1;
                    advance_line();
                    continue;
                }
                skip_dead_line();
                continue;
            }
            if (*p == '#') return &mark_directive;
            if (*p == '\n') {
                lvl->p = p + 1;
                advance_line();
                continue;
            }
            lvl->bol = false;
        }
        Token *t = lex_one(&lvl->p, &lvl->reported_line);
        if (!t) {
            if (!pop_level()) return &mark_eof;
            continue;
        }
        if (t->kind == TK_NL || t->kind == TK_CNL) {
            advance_line();
            lvl->bol = true;
            continue;
        }
        return t;
    }
}

// ============================================================
// Macro expansion over token streams
// ============================================================

typedef struct Frame Frame;
struct Frame {
    Frame *next;
    Token *pos;
    Macro *mac;
    // Location of the macro invocation. Tokens produced by the expansion are
    // restamped with it so diagnostics (and __LINE__) point at the use site,
    // not the macro definition. stamp==false leaves token locations untouched.
    bool stamp;
    char *exp_file;
    int exp_line;
};
typedef struct TNode TNode;
struct TNode {
    TNode *next;
    Token *tok;
};

static Frame *frames;
static Frame *frame_floor; // frame_pull() stops here; keeps expand_list() from draining outer frames
static int nframes;
static TNode *ungot;
static bool xp_in_cond;
static bool xp_no_raw;
static Token *xout_head, *xout_tail;

static void out_append(Token *t) {
    if (!t) return;
    t->next = NULL;
    if (xout_tail) xout_tail->next = t;
    else
        xout_head = t;
    xout_tail = t;
}
static void push_frame(Token *list, Macro *mac) {
    Frame *f = arena_alloc(sizeof(Frame));
    f->pos = list;
    f->mac = mac;
    f->stamp = false;
    f->next = frames;
    frames = f;
    nframes++;
}
// Push a macro expansion whose tokens should be restamped to the invocation site.
static void push_expansion(Token *list, Macro *mac, Token *site) {
    push_frame(list, mac);
    if (site) {
        frames->stamp = true;
        frames->exp_file = site->filename;
        frames->exp_line = site->lineno;
    }
}
static Token *frame_pull(void) {
    while (frames != frame_floor) {
        Frame *top = frames;
        Token *t = top->pos;
        if (!t || t->kind == TK_EOF) {
            frames = top->next;
            nframes--;
            if (top->mac) top->mac->disabled = false;
            continue;
        }
        top->pos = t->next;
        Token *c = copy_token(t);
        if (top->stamp) {
            c->filename = top->exp_file;
            c->lineno = top->exp_line;
        }
        return c;
    }
    return NULL;
}
static void xp_unget(Token *t) {
    TNode *n = arena_alloc(sizeof(TNode));
    n->tok = t;
    n->next = ungot;
    ungot = n;
}
static Token *ungot_pull(void) {
    if (!ungot) return NULL;
    Token *t = ungot->tok;
    ungot = ungot->next;
    return t;
}
static Token *xp_next(void) {
    Token *t = ungot_pull();
    if (t) return t;
    t = frame_pull();
    if (t) return t;
    if (xp_no_raw) return NULL;
    return pp_next_raw();
}

static void expand_token(Token *t);
static void drain_frames(void) {
    for (;;) {
        Token *t = ungot_pull();
        if (!t) t = frame_pull();
        if (!t) break;
        if (t == &mark_eof || t == &mark_directive) {
            xp_unget(t);
            break;
        }
        expand_token(t);
    }
}
static Token *expand_list(Token *list) {
    if (!list) return NULL;
    Token *save_head = xout_head, *save_tail = xout_tail;
    TNode *save_ungot = ungot;
    ungot = NULL;
    bool save_no_raw = xp_no_raw;
    xp_no_raw = true;
    Frame *save_floor = frame_floor;
    frame_floor = frames; // don't let drain_frames() descend into outer frames
    xout_head = xout_tail = NULL;
    push_frame(list, NULL);
    drain_frames();
    frame_floor = save_floor;
    Token *r = xout_head;
    xp_no_raw = save_no_raw;
    ungot = save_ungot;
    xout_head = save_head;
    xout_tail = save_tail;
    return r;
}
static void splice_tokens(Token **head, Token **tail, Token *list) {
    for (; list && list->kind != TK_EOF; list = list->next) {
        Token *c = copy_token(list);
        c->next = NULL;
        if (*tail) (*tail)->next = c;
        else
            *head = c;
        *tail = c;
    }
}
// Append a single token. Body tokens are one long chain, so splice_tokens()
// would over-copy every following body token; use this when only `t` is meant.
static void append_one(Token **head, Token **tail, Token *t) {
    Token *c = copy_token(t);
    c->next = NULL;
    if (*tail) (*tail)->next = c;
    else
        *head = c;
    *tail = c;
}
static Token *pop_tail(Token **head, Token **tail) {
    Token *t = *tail;
    if (!t) return NULL;
    if (*head == t) {
        *head = *tail = NULL;
        return t;
    }
    Token *p = *head;
    while (p->next != t) p = p->next;
    p->next = NULL;
    *tail = p;
    return t;
}
// In stringization (#arg) a run of source whitespace between two argument
// tokens collapses to a single space; adjacent source tokens get none. Source
// adjacency is recoverable from the buffer pointers when both live in the same
// buffer; otherwise (synthesized tokens) default to a separating space.
static bool str_needs_space(Token *a, Token *b) {
    if (!a || !b) return false;
    if (a->no_space_after) return false;
    if (!a->ptr || !b->ptr) return true;
    int al; // spelling length is the source span (t->val for strings, not t->len)
    tok_spelling(a, &al);
    return a->ptr + al != b->ptr;
}
static char *stringize_list(Token *list) {
    int total = 1;
    for (Token *t = list; t && t->kind != TK_EOF; t = t->next) {
        int sl;
        tok_spelling(t, &sl);
        total += sl + 1;
    }
    char *buf = arena_alloc(total);
    int n = 0;
    Token *prev = NULL;
    for (Token *t = list; t && t->kind != TK_EOF; t = t->next) {
        int sl;
        char *sp = tok_spelling(t, &sl);
        if (prev && str_needs_space(prev, t)) buf[n++] = ' ';
        memcpy(buf + n, sp, sl);
        n += sl;
        prev = t;
    }
    buf[n] = '\0';
    return buf;
}
static char *stringize_va(Macro *m, Token **args, int argc) {
    int start = va_slot(m), total = 1;
    for (int i = start; i < argc; i++) {
        for (Token *t = args[i]; t && t->kind != TK_EOF; t = t->next) {
            int sl;
            tok_spelling(t, &sl);
            total += sl + 1;
        }
        total += 1;
    }
    char *buf = arena_alloc(total);
    int n = 0;
    for (int i = start; i < argc; i++) {
        if (i > start) buf[n++] = ',';
        Token *prev = NULL;
        for (Token *t = args[i]; t && t->kind != TK_EOF; t = t->next) {
            int sl;
            char *sp = tok_spelling(t, &sl);
            if (prev && str_needs_space(prev, t)) buf[n++] = ' ';
            memcpy(buf + n, sp, sl);
            n += sl;
            prev = t;
        }
    }
    buf[n] = '\0';
    return buf;
}
static void splice_va(Token **head, Token **tail, Macro *m, Token **args, int argc, Token *site) {
    int start = va_slot(m);
    for (int i = start; i < argc; i++) {
        if (i > start) {
            Token *comma = syn_punct(",", site);
            comma->next = NULL;
            if (*tail) (*tail)->next = comma;
            else
                *head = comma;
            *tail = comma;
        }
        splice_tokens(head, tail, args[i]);
    }
}
static Token *subst_range(Macro *m, Token *body, Token *end, Token **args, Token **raw_args, int argc) {
    Token *rhead = NULL, *rtail = NULL;
    int vs = va_slot(m);
    bool va_empty = vs >= argc || (vs == argc - 1 && !raw_args[vs]);
    for (Token *b = body; b && b != end && b->kind != TK_EOF; b = b->next) {
        int is_hashhash = b->kind == TK_PUNCT && b->len == 2 && b->ptr[0] == '#' && b->ptr[1] == '#';
        if (is_hashhash && b->next && b->next != end && b->next->kind != TK_EOF) {
            Token *n = b->next;
            if (m->is_variadic && n && n != end && n->kind == TK_IDENT && param_or_va(m, n->name) == vs &&
                va_empty && rtail && ptok(rtail, ",")) {
                pop_tail(&rhead, &rtail);
                b = n;
                continue;
            }
            Token *xhead = NULL, *xtail = NULL;
            if (n && n != end && n->kind != TK_EOF && n->kind == TK_IDENT) {
                int idx = param_or_va(m, n->name);
                if (m->is_variadic && idx == vs) {
                    // Variadic slot: splice_va handles argc <= vs (no
                    // trailing args supplied at all) by producing nothing,
                    // same as an explicitly empty variadic argument.
                    splice_va(&xhead, &xtail, m, raw_args, argc, n);
                } else if (idx >= 0 && idx < argc) {
                    splice_tokens(&xhead, &xtail, raw_args[idx]);
                } else
                    append_one(&xhead, &xtail, n);
                b = n;
            } else if (n && n != end && n->kind != TK_EOF) {
                append_one(&xhead, &xtail, n);
                b = n;
            }
            if (!xhead) continue;
            if (!rtail) {
                rhead = xhead;
                rtail = xtail;
                continue;
            }
            Token *lhs = pop_tail(&rhead, &rtail);
            int l1, l2;
            char *s1 = tok_spelling(lhs, &l1), *s2 = tok_spelling(xhead, &l2);
            char *pasted = arena_alloc(l1 + l2 + 1);
            memcpy(pasted, s1, l1);
            memcpy(pasted + l1, s2, l2);
            pasted[l1 + l2] = '\0';
            Token *pt = lex_body_string(pasted, lhs->filename, lhs->lineno);
            // pt's spelling lives in a freshly lexed buffer, so the normal
            // pointer-adjacency check in str_needs_space() can never see it
            // as touching whatever follows — even when the macro body had
            // zero whitespace between the ## operator's rhs operand and the
            // next body token (e.g. "__export_symbol_##sym:"). Recover that
            // from the pre-substitution body tokens (n, n->next) so a later
            // #-stringize of this expansion keeps them adjacent too.
            if (!xhead->next && n->next && !str_needs_space(n, n->next))
                pt->no_space_after = true;
            splice_tokens(&rhead, &rtail, pt);
            splice_tokens(&rhead, &rtail, xhead->next);
            continue;
        }
        if (ptok(b, "#")) {
            Token *n = b->next;
            if (n && n != end && n->kind != TK_EOF && n->kind == TK_IDENT) {
                int idx = param_or_va(m, n->name);
                if (idx >= 0) {
                    char *s = (m->is_variadic && idx == vs) ? stringize_va(m, args, argc) : (idx < argc ? stringize_list(raw_args[idx]) : "");
                    Token *st = syn_str(s, strlen(s), b);
                    st->next = NULL;
                    if (rtail) rtail->next = st;
                    else
                        rhead = st;
                    rtail = st;
                    b = n;
                    continue;
                }
            }
            Token *c = copy_token(b);
            c->next = NULL;
            if (rtail) rtail->next = c;
            else
                rhead = c;
            rtail = c;
            continue;
        }
        if (b->kind == TK_IDENT) {
            char *bn = b->name;
            if (m->is_variadic && bn == kw_va_args) {
                splice_va(&rhead, &rtail, m, (m->hh_mask & (1u << vs)) ? raw_args : args, argc, b);
                continue;
            }
            if (m->is_variadic && bn == kw_va_opt) {
                Token *o = b->next;
                if (o && o != end && ptok(o, "(")) {
                    int depth = 1;
                    Token *c = o->next;
                    while (c && c != end && c->kind != TK_EOF && depth > 0) {
                        if (ptok(c, "(")) depth++;
                        else if (ptok(c, ")"))
                            depth--;
                        if (depth > 0) c = c->next;
                    }
                    if (!va_empty) {
                        Token *sub = subst_range(m, o->next, c, args, raw_args, argc);
                        splice_tokens(&rhead, &rtail, sub);
                    }
                    b = c;
                    continue;
                }
                append_one(&rhead, &rtail, b);
                continue;
            }
            int idx = find_param_index(m, bn);
            if (m->is_variadic && idx == vs) {
                // Named GNU variadic param (`args...`) with no trailing
                // arguments supplied: splice_va yields nothing, same as
                // literal __VA_ARGS__ above.
                splice_va(&rhead, &rtail, m, (m->hh_mask & (1u << idx)) ? raw_args : args, argc, b);
                continue;
            }
            if (idx >= 0 && idx < argc) {
                // The substituted argument's spelling lives in whatever
                // buffer the call site's actual argument came from, so the
                // normal pointer-adjacency check (str_needs_space(), and
                // pp_tokens_to_text()'s own copy of the same idea) can never
                // see it as touching whatever body token follows — even
                // when the macro body itself had zero whitespace between
                // the parameter and the next token (e.g. linkage.h's
                // "name:" in "SYM_ENTRY(name, ...) ... name:", a label built
                // from a macro parameter). Recover that from the
                // pre-substitution body tokens (b, b->next), same trick
                // already used above for explicit "##" pastes.
                Token *before = rtail;
                splice_tokens(&rhead, &rtail, (m->hh_mask & (1u << idx)) ? raw_args[idx] : args[idx]);
                if (rtail && rtail != before && b->next && b->next != end &&
                    b->next->kind != TK_EOF && !str_needs_space(b, b->next))
                    rtail->no_space_after = true;
                continue;
            }
            append_one(&rhead, &rtail, b);
            continue;
        }
        Token *c = copy_token(b);
        c->next = NULL;
        if (rtail) rtail->next = c;
        else
            rhead = c;
        rtail = c;
    }
    return rhead;
}
// Actual-argument capacity for a function-like macro call. Separate from (and
// larger than) the formal-parameter cap on #define itself: variadic wrapper
// macros like the kernel's "#define PARAMS(args...) args" are invoked with
// however many comma-separated tokens the caller's own expanded arguments
// happen to contain (e.g. TRACE_EVENT's PARAMS(proto) after proto's own
// TP_PROTO(...) has argument-prescan-expanded into a bare list), easily
// exceeding a small fixed cap even though the macro itself takes "any number
// of args".
#define MAX_CALL_ARGS 128
static void expand_token(Token *t) {
    if (!t || t->kind != TK_IDENT) {
        out_append(t);
        return;
    }
    char *name = t->name;
    if (!name) {
        out_append(t);
        return;
    }
    if (name == kw_line) {
        out_append(syn_num(pp_cur_line ? pp_cur_line : t->lineno, t));
        return;
    }
    if (name == kw_file) {
        char *fn = pp_cur_file ? pp_cur_file : (t->filename ? t->filename : "");
        out_append(syn_str(fn, strlen(fn), t));
        return;
    }
    if (name == kw_counter) {
        out_append(syn_num(pp_counter++, t));
        return;
    }
    if (name == kw_function || name == kw_func || name == kw_pretty_function) {
        out_append(t);
        return;
    }
    if (name == kw_has_include || name == kw_has_include_next || name == kw_has_c_attribute) {
        Token *nx = xp_next();
        if (ptok(nx, "(")) {
            xp_unget(nx);
            out_append(t);
            return;
        }
        xp_unget(nx);
    }
    if (xp_in_cond && name == kw_defined) {
        out_append(t);
        Token *nx = xp_next();
        if (ptok(nx, "(")) {
            out_append(nx);
            Token *id = xp_next();
            if (id && id->kind == TK_IDENT) out_append(id);
            else
                xp_unget(id);
            Token *rp = xp_next();
            if (ptok(rp, ")")) out_append(rp);
            else
                xp_unget(rp);
        } else if (nx && nx->kind == TK_IDENT)
            out_append(nx);
        else
            xp_unget(nx);
        return;
    }
    Macro *m = find_macro_interned(name);
    if (!m || m->disabled) {
        out_append(t);
        return;
    }
    if (!m->is_function) {
        if (nframes > 600) {
            out_append(t);
            return;
        }
        m->disabled = true;
        push_expansion(m->body, m, t);
        return;
    }
    Token *lp = xp_next();
    if (!ptok(lp, "(")) {
        xp_unget(lp);
        out_append(t);
        return;
    }
    if (nframes > 600) {
        xp_unget(lp);
        out_append(t);
        return;
    }
    Token *args[MAX_CALL_ARGS] = {}, *tails[MAX_CALL_ARGS] = {};
    int argc = 0, depth = 1;
    bool any = false;
    Token *rp = NULL;
    for (;;) {
        Token *x = xp_next();
        if (x == &mark_directive) {
            // GNU extension: a conditional-compilation directive is allowed
            // in the middle of a function-like macro call's argument list
            // (e.g. the kernel's struct_group(NAME, ...#ifdef CONFIG_X...)).
            // Process it in place — it only flips pp_active()/lvl->conds
            // state — then keep collecting arguments across it, instead of
            // giving up on the whole invocation as if it were never called.
            do_directive();
            continue;
        }
        if (x == &mark_eof) {
            xp_unget(x);
            int filled = argc + (any ? 1 : 0);
            Token *replay = NULL, *reptail = NULL;
            for (int i = 0; i < filled; i++) {
                if (i > 0) {
                    Token *comma = syn_punct(",", t);
                    comma->next = NULL;
                    if (reptail) reptail->next = comma;
                    else
                        replay = comma;
                    reptail = comma;
                }
                splice_tokens(&replay, &reptail, args[i]);
            }
            Token *rev = NULL;
            while (replay) {
                Token *n = replay->next;
                replay->next = rev;
                rev = replay;
                replay = n;
            }
            while (rev) {
                Token *n = rev->next;
                xp_unget(rev);
                rev = n;
            }
            xp_unget(lp);
            out_append(t);
            return;
        }
        if (ptok(x, "(")) depth++;
        else if (ptok(x, ")")) {
            if (--depth == 0) {
                rp = x;
                break;
            }
        } else if (ptok(x, ",") && depth == 1) {
            if (argc >= MAX_CALL_ARGS) error_tok(t, "too many macro arguments (in %s)", name);
            argc++;
            any = false;
            continue;
        }
        if (argc < MAX_CALL_ARGS) {
            x->next = NULL;
            if (tails[argc]) tails[argc]->next = x;
            else
                args[argc] = x;
            tails[argc] = x;
            any = true;
        }
    }
    if (any || argc > 0) argc++;
    if (!m->is_variadic && argc > m->param_len) {
        out_append(t);
        out_append(lp);
        for (int i = 0; i < argc; i++) {
            if (i > 0) out_append(syn_punct(",", t));
            splice_tokens(&xout_head, &xout_tail, args[i]);
        }
        out_append(rp);
        return;
    }
    while (argc < m->param_len) args[argc++] = NULL;
    Token *exp_args[MAX_CALL_ARGS] = {};
    Token *args_copy[MAX_CALL_ARGS] = {};
    for (int i = 0; i < argc; i++) {
        Token *h = NULL, *t_ = NULL;
        splice_tokens(&h, &t_, args[i]);
        args_copy[i] = h;
    }
    // hh_mask bits only correspond to formal (#define-side) parameter
    // positions, which are capped at 32; guard i < 32 so an actual argument
    // index beyond that (only possible for a variadic call) can't alias
    // into a low bit via an out-of-range shift.
    for (int i = 0; i < argc; i++) exp_args[i] = (i < 32 && (m->hh_mask & (1u << i))) ? args_copy[i] : expand_list(args_copy[i]);
    Token *subst = subst_range(m, m->body, NULL, exp_args, args, argc);
    m->disabled = true;
    push_expansion(subst, m, t);
}

// ============================================================
// #if expression evaluator
// ============================================================

static bool pp_expr_unsigned;
static char *gather_spellings(Token *t, const char *close, Token **rest) {
    int clen = strlen(close);
    int total = 1;
    Token *q;
    for (q = t; q && q->kind != TK_EOF && !(q->kind == TK_PUNCT && q->len == clen && !memcmp(q->ptr, close, clen)); q = q->next) {
        int sl;
        tok_spelling(q, &sl);
        total += sl;
    }
    char *buf = arena_alloc(total);
    int n = 0;
    for (q = t; q && q->kind != TK_EOF && !(q->kind == TK_PUNCT && q->len == clen && !memcmp(q->ptr, close, clen)); q = q->next) {
        int sl;
        char *sp = tok_spelling(q, &sl);
        memcpy(buf + n, sp, sl);
        n += sl;
    }
    buf[n] = '\0';
    if (rest) *rest = q;
    return buf;
}
static void normalize_attr_name(char *s) {
    size_t n = strlen(s);
    if (n >= 4 && s[0] == '_' && s[1] == '_' && s[n - 1] == '_' && s[n - 2] == '_') {
        memmove(s, s + 2, n - 4);
        s[n - 4] = '\0';
    }
}
static int64_t has_c_attribute_val(char *name) {
    char ns[64] = "", at[64] = "";
    char *colon = strstr(name, "::");
    if (colon) {
        int nl = (int)(colon - name);
        while (nl > 0 && isspace((unsigned char)name[nl - 1])) nl--;
        if (nl > 63) nl = 63;
        memcpy(ns, name, nl);
        ns[nl] = '\0';
        char *ap = colon + 2;
        while (isspace((unsigned char)*ap)) ap++;
        int al = 0;
        while (ap[al] && !isspace((unsigned char)ap[al]) && al < 63) al++;
        memcpy(at, ap, al);
        at[al] = '\0';
    } else {
        int al = 0;
        while (name[al] && !isspace((unsigned char)name[al]) && al < 63) al++;
        memcpy(at, name, al);
        at[al] = '\0';
    }
    normalize_attr_name(ns);
    normalize_attr_name(at);
    if (ns[0]) {
        if (strcmp(ns, "gnu") == 0) {
            static const char *gnu_attrs[] = {"packed", "aligned", "always_inline", "noinline", "noreturn", "unused",
                                              "used", "deprecated", "const", "pure", "malloc", "cold", "hot", "constructor", "destructor", "weak", "alias",
                                              "cleanup", "nonnull", "returns_nonnull", "warn_unused_result", "sentinel", "format", "transparent_union",
                                              "vector_size", "may_alias", "visibility", "section", "fallthrough", NULL};
            for (int i = 0; gnu_attrs[i]; i++)
                if (strcmp(at, gnu_attrs[i]) == 0) return 1;
        }
        return 0;
    }
    static const char *std_attrs[] = {"deprecated", "fallthrough", "maybe_unused", "nodiscard", "noreturn",
                                      "_Noreturn", "unsequenced", "reproducible", NULL};
    for (int i = 0; std_attrs[i]; i++)
        if (strcmp(at, std_attrs[i]) == 0) return 202311L;
    return 0;
}
static int64_t eval_pp_expr_tok(Token **pp);
static int64_t eval_primary_tok(Token **pp) {
    Token *t = *pp;
    if (!t || t->kind == TK_EOF) return 0;
    if (t->kind == TK_IDENT) {
        char *nm = t->name;
        if (nm == kw_defined) {
            t = t->next;
            bool paren = ptok(t, "(");
            if (paren) t = t->next;
            int64_t r = 0;
            if (t && t->kind == TK_IDENT) {
                r = find_macro_interned(t->name) != NULL;
                t = t->next;
            }
            if (paren && ptok(t, ")")) t = t->next;
            *pp = t;
            return r;
        }
        if (nm == kw_has_include || nm == kw_has_include_next) {
            t = t->next;
            if (ptok(t, "(")) t = t->next;
            char *spec = NULL;
            bool is_angle = false;
            if (t && t->kind == TK_STR) {
                spec = str_raw_contents(t);
                t = t->next;
                int sl = strlen(spec);
                if (sl >= 2 && spec[0] == '<' && spec[sl - 1] == '>') {
                    is_angle = true;
                    spec[sl - 1] = '\0';
                    spec++;
                }
            } else if (ptok(t, "<")) {
                is_angle = true;
                spec = gather_spellings(t->next, ">", &t);
                if (ptok(t, ">")) t = t->next;
            } else {
                spec = gather_spellings(t, ")", &t);
                int sl = strlen(spec);
                if (sl >= 2 && spec[0] == '"' && spec[sl - 1] == '"') {
                    spec[sl - 1] = '\0';
                    spec++;
                } else if (sl >= 2 && spec[0] == '<' && spec[sl - 1] == '>') {
                    is_angle = true;
                    spec[sl - 1] = '\0';
                    spec++;
                }
            }
            if (ptok(t, ")")) t = t->next;
            *pp = t;
            return spec ? resolve_include(lvl->filename, spec, is_angle) != NULL : 0;
        }
        if (nm == kw_has_c_attribute) {
            t = t->next;
            if (ptok(t, "(")) t = t->next;
            char *text = gather_spellings(t, ")", &t);
            if (ptok(t, ")")) t = t->next;
            *pp = t;
            return has_c_attribute_val(text);
        }
        if (opt_std_version && strcmp(opt_std_version, "202311L") == 0) {
            if (nm == kw_true) {
                *pp = t->next;
                return 1;
            }
            if (nm == kw_false) {
                *pp = t->next;
                return 0;
            }
        }
        *pp = t->next;
        return 0;
    }
    if (t->kind == TK_NUM) {
        for (int i = t->len; i > 0; i--) {
            char c = t->ptr[i - 1];
            if (c == 'u' || c == 'U') pp_expr_unsigned = true;
            else if (c != 'l' && c != 'L')
                break;
        }
        if (t->string_literal_prefix && t->string_literal_prefix != 'L') pp_expr_unsigned = true;
        *pp = t->next;
        return t->val;
    }
    if (t->kind == TK_FNUM) {
        *pp = t->next;
        return (int64_t)t->fval;
    }
    if (t->kind == TK_STR) {
        *pp = t->next;
        return 0;
    }
    if (ptok(t, "(")) {
        t = t->next;
        int64_t val = eval_pp_expr_tok(&t);
        if (ptok(t, ")")) t = t->next;
        *pp = t;
        return val;
    }
    if (ptok(t, "!")) {
        t = t->next;
        int64_t val = !eval_primary_tok(&t);
        *pp = t;
        return val;
    }
    if (ptok(t, "-")) {
        t = t->next;
        int64_t val = -eval_primary_tok(&t);
        *pp = t;
        return val;
    }
    *pp = t->next;
    return 0;
}
static int64_t eval_mul_tok(Token **pp) {
    int64_t val = eval_primary_tok(pp);
    for (;;) {
        Token *t = *pp;
        if (ptok(t, "*")) {
            t = t->next;
            val *= eval_primary_tok(&t);
        } else if (ptok(t, "/")) {
            t = t->next;
            int64_t rhs = eval_primary_tok(&t);
            val = rhs ? val / rhs : 0;
        } else if (ptok(t, "%")) {
            t = t->next;
            int64_t rhs = eval_primary_tok(&t);
            val = rhs ? val % rhs : 0;
        } else
            break;
        *pp = t;
    }
    return val;
}
static int64_t eval_add_tok(Token **pp) {
    int64_t val = eval_mul_tok(pp);
    for (;;) {
        Token *t = *pp;
        if (ptok(t, "+")) {
            t = t->next;
            val += eval_mul_tok(&t);
        } else if (ptok(t, "-")) {
            t = t->next;
            val -= eval_mul_tok(&t);
        } else
            break;
        *pp = t;
    }
    return val;
}
static int64_t eval_shift_tok(Token **pp) {
    int64_t val = eval_add_tok(pp);
    for (;;) {
        Token *t = *pp;
        if (ptok(t, "<<")) {
            t = t->next;
            val = val << eval_add_tok(&t);
        } else if (ptok(t, ">>")) {
            t = t->next;
            val = val >> eval_add_tok(&t);
        } else
            break;
        *pp = t;
    }
    return val;
}
static int64_t eval_rel_tok(Token **pp) {
    int64_t val = eval_shift_tok(pp);
    for (;;) {
        Token *t = *pp;
        int64_t rhs;
        if (ptok(t, "<=")) {
            t = t->next;
            rhs = eval_shift_tok(&t);
            val = pp_expr_unsigned ? (uint64_t)val <= (uint64_t)rhs : val <= rhs;
        } else if (ptok(t, ">=")) {
            t = t->next;
            rhs = eval_shift_tok(&t);
            val = pp_expr_unsigned ? (uint64_t)val >= (uint64_t)rhs : val >= rhs;
        } else if (ptok(t, "<")) {
            t = t->next;
            rhs = eval_shift_tok(&t);
            val = pp_expr_unsigned ? (uint64_t)val < (uint64_t)rhs : val < rhs;
        } else if (ptok(t, ">")) {
            t = t->next;
            rhs = eval_shift_tok(&t);
            val = pp_expr_unsigned ? (uint64_t)val > (uint64_t)rhs : val > rhs;
        } else
            break;
        *pp = t;
    }
    return val;
}
static int64_t eval_eq_tok(Token **pp) {
    int64_t val = eval_rel_tok(pp);
    for (;;) {
        Token *t = *pp;
        if (ptok(t, "==")) {
            t = t->next;
            val = val == eval_rel_tok(&t);
        } else if (ptok(t, "!=")) {
            t = t->next;
            val = val != eval_rel_tok(&t);
        } else
            break;
        *pp = t;
    }
    return val;
}
static int64_t eval_bitand_tok(Token **pp) {
    int64_t val = eval_eq_tok(pp);
    Token *t;
    while (ptok((t = *pp), "&") && !ptok(t->next, "&")) {
        t = t->next;
        val &= eval_eq_tok(&t);
        *pp = t;
    }
    return val;
}
static int64_t eval_bitxor_tok(Token **pp) {
    int64_t val = eval_bitand_tok(pp);
    Token *t;
    while (ptok((t = *pp), "^")) {
        t = t->next;
        val ^= eval_bitand_tok(&t);
        *pp = t;
    }
    return val;
}
static int64_t eval_bitor_tok(Token **pp) {
    int64_t val = eval_bitxor_tok(pp);
    Token *t;
    while (ptok((t = *pp), "|") && !ptok(t->next, "|")) {
        t = t->next;
        val |= eval_bitxor_tok(&t);
        *pp = t;
    }
    return val;
}
static int64_t eval_land_tok(Token **pp) {
    int64_t val = eval_bitor_tok(pp);
    Token *t;
    while (ptok((t = *pp), "&&")) {
        t = t->next;
        int64_t rhs = eval_bitor_tok(&t);
        val = val && rhs;
        *pp = t;
    }
    return val;
}
static int64_t eval_pp_expr_tok(Token **pp) {
    int64_t val = eval_land_tok(pp);
    Token *t;
    while (ptok((t = *pp), "||")) {
        t = t->next;
        int64_t rhs = eval_land_tok(&t);
        val = val || rhs;
        *pp = t;
    }
    t = *pp;
    if (ptok(t, "?")) {
        t = t->next;
        int64_t tv = eval_pp_expr_tok(&t);
        if (ptok(t, ":")) t = t->next;
        int64_t fv = eval_pp_expr_tok(&t);
        val = val ? tv : fv;
        *pp = t;
    }
    return val;
}
static int64_t eval_condition_tok(Token *expr) {
    pp_expr_unsigned = false;
    xp_in_cond = true;
    Token *expanded = expand_list(expr);
    xp_in_cond = false;
    Token *cur = expanded;
    return eval_pp_expr_tok(&cur);
}

// ============================================================
// Directives
// ============================================================

static void cond_if(int64_t val) {
    CondIncl *ci = arena_alloc(sizeof(CondIncl));
    ci->parent_active = pp_active();
    ci->active = ci->parent_active && (val != 0);
    ci->branch_taken = ci->active;
    ci->next = lvl->conds;
    lvl->conds = ci;
}
static void emit_pragma_marker(char *what, int value, bool has_value, Token *site) {
    out_append(syn_punct("#", site));
    out_append(syn_ident("pragma", site));
    out_append(syn_ident(what, site));
    out_append(syn_punct("(", site));
    if (has_value) out_append(syn_num(value, site));
    out_append(syn_punct(")", site));
}
static Token *collect_directive_tokens(char *p, int *pln, Token **name_out) {
    Token head = {};
    Token *tail = &head;
    *name_out = NULL;
    for (;;) {
        Token *t = lex_one(&p, pln);
        if (!t) break;
        if (t->kind == TK_NL || t->kind == TK_CNL) {
            advance_line();
            break;
        }
        tail = tail->next = t;
        if (!*name_out && t->kind == TK_IDENT) *name_out = t;
    }
    tail->next = NULL;
    lvl->p = p;
    return head.next;
}
static void do_directive(void) {
    Token *name = NULL;
    Token *body = collect_directive_tokens(lvl->p, &lvl->reported_line, &name);
    (void)body;
    if (!name) return;
    char *dn = name->name;
    if (dn == dn_if) {
        cond_if(pp_active() ? eval_condition_tok(name->next) : 0);
        return;
    }
    if (dn == dn_ifdef) {
        Token *id = name->next;
        cond_if(id && id->kind == TK_IDENT ? find_macro_interned(id->name) != NULL : false);
        return;
    }
    if (dn == dn_ifndef) {
        Token *id = name->next;
        cond_if(id && id->kind == TK_IDENT ? find_macro_interned(id->name) == NULL : true);
        return;
    }
    if (dn == dn_elif) {
        if (lvl->conds) {
            if (!lvl->conds->branch_taken) {
                lvl->conds->active = lvl->conds->parent_active && eval_condition_tok(name->next);
                if (lvl->conds->active) lvl->conds->branch_taken = true;
            } else
                lvl->conds->active = false;
        }
        return;
    }
    if (dn == dn_elifdef) {
        if (lvl->conds) {
            if (!lvl->conds->branch_taken) {
                Token *id = name->next;
                lvl->conds->active = lvl->conds->parent_active && (id && id->kind == TK_IDENT ? find_macro_interned(id->name) != NULL : false);
                if (lvl->conds->active) lvl->conds->branch_taken = true;
            } else
                lvl->conds->active = false;
        }
        return;
    }
    if (dn == dn_elifndef) {
        if (lvl->conds) {
            if (!lvl->conds->branch_taken) {
                Token *id = name->next;
                lvl->conds->active = lvl->conds->parent_active && (id && id->kind == TK_IDENT ? find_macro_interned(id->name) == NULL : true);
                if (lvl->conds->active) lvl->conds->branch_taken = true;
            } else
                lvl->conds->active = false;
        }
        return;
    }
    if (dn == dn_else) {
        if (lvl->conds) {
            lvl->conds->active = lvl->conds->parent_active && !lvl->conds->branch_taken;
            lvl->conds->branch_taken = true;
        }
        return;
    }

    if (dn == dn_endif) {
        if (lvl->conds) lvl->conds = lvl->conds->next;
        return;
    }
    if (!pp_active()) return;
    if (dn == dn_define) {
        Token *name_tok = name->next, *rest = name_tok ? name_tok->next : NULL;
        if (!name_tok || name_tok->kind != TK_IDENT) return;
        char *mname = name_tok->name;
        pp_check_ident(mname, name_tok->len, lvl->filename, lvl->reported_line);
        bool is_function = false, is_variadic = false, is_gnu_variadic = false;
        char *params[32];
        int np = 0;
        Token *b = rest;
        if (b && ptok(b, "(") && b->ptr == name_tok->ptr + name_tok->len) {
            is_function = true;
            Token *prev = NULL;
            b = b->next;
            while (b && !ptok(b, ")")) {
                if (ptok(b, "...")) {
                    is_variadic = true;
                    if (prev && prev->kind == TK_IDENT && np > 0) is_gnu_variadic = true;
                    prev = b;
                    b = b->next;
                    continue;
                }
                if (b->kind == TK_IDENT) {
                    if (np < 32) params[np++] = b->name;
                    prev = b;
                    b = b->next;
                    continue;
                }
                prev = b;
                b = b->next;
            }
            if (b && ptok(b, ")")) b = b->next;
        }
        Token *mbody = NULL, *btail = NULL;
        splice_tokens(&mbody, &btail, b);
        char **pc = NULL;
        if (np > 0) {
            pc = arena_alloc(sizeof(char *) * np);
            memcpy(pc, params, sizeof(char *) * np);
        }
        define_macro_tok(mname, is_function, pc, np, mbody, is_variadic, is_gnu_variadic);
        return;
    }
    if (dn == dn_undef) {
        Token *id = name->next;
        if (id && id->kind == TK_IDENT) {
            pp_check_ident(id->name, id->len, lvl->filename, lvl->reported_line);
            add_undef(id->name);
        }
        return;
    }
    if (dn == dn_include) {
        Token *first = name->next;
        char *spec = NULL;
        bool is_angle = false;
        if (ptok(first, "<")) {
            is_angle = true;
            spec = gather_spellings(first->next, ">", NULL);
        } else if (first && first->kind == TK_STR)
            spec = str_raw_contents(first);
        else if (first && first->kind != TK_EOF) {
            Token *exp = expand_list(first);
            if (exp && exp->kind == TK_STR) spec = str_raw_contents(exp);
            else if (ptok(exp, "<")) {
                is_angle = true;
                spec = gather_spellings(exp->next, ">", NULL);
            }
        }
        if (!spec) return;
        char *path = resolve_include(lvl->fpath, spec, is_angle);
        if (!path) {
            fprintf(stderr, "%s:%d: error: include file '%s' not found\n", lvl->fpath, lvl->reported_line, spec);
            exit(1);
        }
        char *inc_fpath = full_path(path);
        if (is_once_file(inc_fpath)) return;
        char *contents = read_pp_file(path);
        if (!contents) {
            fprintf(stderr, "%s:%d: error: cannot read include file '%s'\n", lvl->fpath, lvl->reported_line, path);
            exit(1);
        }
        push_level(inc_fpath, inc_fpath, contents);
        dep_add(inc_fpath);
        return;
    }
    if (dn == dn_line) {
        Token *rest = expand_list(name->next);
        if (!rest || rest->kind != TK_NUM) return;
        // The #line argument is a decimal digit-sequence, not a C integer
        // constant: a leading 0 is not octal, and C23 digit separators are
        // allowed. Parse the spelling instead of trusting the lexed value.
        int sl;
        char *sp = tok_spelling(rest, &sl);
        int nl = 0;
        for (int i = 0; i < sl; i++) {
            if (sp[i] == '\'') continue;
            if (sp[i] < '0' || sp[i] > '9') break;
            nl = nl * 10 + (sp[i] - '0');
        }
        if (nl < 1) nl = 1;
        lvl->reported_line = nl;
        Token *fn = rest->next;
        if (fn && fn->kind == TK_STR) {
            lvl->filename = str_raw_contents(fn);
            // Raw tokens are stamped with current_debug_filename by the lexer, so
            // retarget it too or __FILE__ keeps reporting the real file.
            current_debug_filename = lvl->filename;
        }
        return;
    }
    if (dn == dn_error) {
        int total = 0;
        for (Token *t = name->next; t; t = t->next) {
            int sl;
            tok_spelling(t, &sl);
            total += sl + 1;
        }
        char *msg = arena_alloc(total + 1);
        int n = 0;
        for (Token *t = name->next; t; t = t->next) {
            int sl;
            char *sp = tok_spelling(t, &sl);
            if (n > 0) msg[n++] = ' ';
            memcpy(msg + n, sp, sl);
            n += sl;
        }
        msg[n] = '\0';
        fprintf(stderr, "%s:%d: error: %s\n", lvl->filename, lvl->reported_line, msg);
        exit(1);
    }
    if (dn == dn_warning) {
        int total = 0;
        for (Token *t = name->next; t; t = t->next) {
            int sl;
            tok_spelling(t, &sl);
            total += sl + 1;
        }
        char *msg = arena_alloc(total + 1);
        int n = 0;
        for (Token *t = name->next; t; t = t->next) {
            int sl;
            char *sp = tok_spelling(t, &sl);
            if (n > 0) msg[n++] = ' ';
            memcpy(msg + n, sp, sl);
            n += sl;
        }
        msg[n] = '\0';
        fprintf(stderr, "%s:%d: warning: %s\n", lvl->filename, lvl->reported_line, msg);
        return;
    }
    if (dn == dn_pragma) {
        Token *p = name->next;
        if (!p || p->kind != TK_IDENT) return;
        char *n = p->name;
        if (!strcmp(n, "once")) {
            mark_once_file(lvl->fpath);
            return;
        }
        if (!strcmp(n, "pack")) {
            p = p->next;
            if (ptok(p, "(")) p = p->next;
            if (p && p->kind == TK_IDENT && !strcmp(p->name, "push")) {
                pack_align_stack[pack_align_idx++] = pack_align;
                p = p->next;
                if (ptok(p, ",")) p = p->next;
                if (p && p->kind == TK_NUM && p->ptr[0] >= '1' && p->ptr[0] <= '9') pack_align = p->ptr[0] - '0';
                emit_pragma_marker("pack", pack_align, true, p);
            } else if (p && p->kind == TK_IDENT && !strcmp(p->name, "pop")) {
                if (pack_align_idx > 0) pack_align = pack_align_stack[--pack_align_idx];
                emit_pragma_marker("pack", pack_align, true, p);
            } else if (p && p->kind == TK_NUM && p->ptr[0] >= '1' && p->ptr[0] <= '9') {
                pack_align = p->ptr[0] - '0';
                emit_pragma_marker("pack", pack_align, true, p);
            } else {
                // Bare `#pragma pack()` (no push/pop/number, e.g. every
                // ACPI table header's pack(1) ... pack() pair) or an
                // explicit `#pragma pack(0)`: both reset to the compiler's
                // default alignment. Falling through here silently (the
                // previous behavior) never reset pack_align, leaking the
                // most recent pack(N) into every struct declared for the
                // rest of the translation unit.
                pack_align = 0;
                emit_pragma_marker("pack", pack_align, true, p);
            }
            return;
        }
        if (!strcmp(n, "FENV_ACCESS")) {
            p = p->next;
            if (p && p->kind == TK_IDENT && !strcmp(p->name, "ON")) fenv_access = true;
            else if (p && p->kind == TK_IDENT && !strcmp(p->name, "OFF"))
                fenv_access = false;
            else if (p && p->kind == TK_IDENT && !strcmp(p->name, "DEFAULT"))
                fenv_access = false;
            emit_pragma_marker("fenv", fenv_access ? 1 : 0, true, p);
            return;
        }
        if (!strcmp(n, "push_macro")) {
            p = p->next;
            if (ptok(p, "(")) p = p->next;
            if (p && p->kind == TK_STR) { push_macro(str_raw_contents(p)); }
            return;
        }
        if (!strcmp(n, "pop_macro")) {
            p = p->next;
            if (ptok(p, "(")) p = p->next;
            if (p && p->kind == TK_STR) { pop_macro(str_raw_contents(p)); }
            return;
        }
        return;
    }
}

// ============================================================
// String concatenation, -E output, -dM
// ============================================================

static bool can_concat_strings(Token *a, Token *b) {
    if (!a || !b) return false;
    if (a->kind != TK_STR || b->kind != TK_STR) return false;
    char pa = a->string_literal_prefix ? a->string_literal_prefix : 0, pb = b->string_literal_prefix ? b->string_literal_prefix : 0;
    // Like L"a" "b" -> L"ab": an unprefixed literal adopts the other's prefix.
    return pa == pb || pa == 0 || pb == 0;
}
static Token *concat_strings(Token *tok) {
    Token head = {};
    Token *tail = &head;
    for (Token *t = tok; t && t->kind != TK_EOF;) {
        if (t->kind == TK_STR && can_concat_strings(t, t->next)) {
            // Greedily absorb every following concatenable string literal, not
            // just the next one, so "a" "b" "c" "d" collapse to a single token.
            int total = t->len;
            for (Token *q = t->next; can_concat_strings(t, q); q = q->next) total += q->len;
            char *merged = arena_alloc(total);
            int mlen = t->len;
            int pfx = t->string_literal_prefix;
            memcpy(merged, t->str ? t->str : "", t->len);
            Token *q = t->next;
            for (; can_concat_strings(t, q); q = q->next) {
                memcpy(merged + mlen, q->str ? q->str : "", q->len);
                mlen += q->len;
                if (!pfx) pfx = q->string_literal_prefix;
            }
            Token *n = copy_token(t);
            n->str = str_intern(merged, mlen);
            n->len = mlen;
            n->string_literal_prefix = pfx;
            int sp_cap = mlen * 2 + 3;
            char *sp = arena_alloc(sp_cap);
            int sn = 0;
            sp[sn++] = '"';
            for (int i = 0; i < mlen; i++) {
                if (merged[i] == '"' || merged[i] == '\\') sp[sn++] = '\\';
                sp[sn++] = merged[i];
            }
            sp[sn++] = '"';
            n->ptr = sp;
            n->val = sn;
            tail = tail->next = n;
            t = q;
            continue;
        }
        // Check if current token merges with the previous output token
        // (handles "a" MACRO "c" where MACRO expands to "b").
        if (t->kind == TK_STR && tail != &head && tail->kind == TK_STR && can_concat_strings(tail, t)) {
            int len1 = tail->len, len2 = t->len;
            char *merged = arena_alloc(len1 + len2);
            memcpy(merged, tail->str ? tail->str : "", len1);
            memcpy(merged + len1, t->str ? t->str : "", len2);
            tail->str = str_intern(merged, len1 + len2);
            tail->len = len1 + len2;
            if (!tail->string_literal_prefix) tail->string_literal_prefix = t->string_literal_prefix;
            // Rebuild quoted spelling
            int sp_cap = (len1 + len2) * 2 + 3;
            char *sp = arena_alloc(sp_cap);
            int sn = 0;
            sp[sn++] = '"';
            for (int i = 0; i < len1 + len2; i++) {
                if (merged[i] == '"' || merged[i] == '\\') sp[sn++] = '\\';
                sp[sn++] = merged[i];
            }
            sp[sn++] = '"';
            tail->ptr = sp;
            tail->val = sn;
            t = t->next;
            continue;
        }
        tail = tail->next = copy_token(t);
        t = t->next;
    }
    // Preserve the terminating EOF (the loop above stops before it). This also
    // covers an empty translation unit, where tok itself is the EOF: the parser
    // dereferences the returned list, so it must never come back NULL.
    Token *eof = tok;
    while (eof && eof->kind != TK_EOF) eof = eof->next;
    if (eof) tail = tail->next = copy_token(eof);
    tail->next = NULL;
    return head.next;
}
// Re-render a preprocessed token stream back to flat text, the same way
// pp_print_tokens() does for -E output, but into a heap buffer instead of
// stdout — used to feed a standalone .S file's preprocessed content
// (macros expanded, #ifdef/#include resolved) into the assembler, which
// only ever consumes plain assembly text.
char *pp_tokens_to_text(Token *tok) {
    size_t cap = 4096, len = 0;
    char *buf = malloc(cap);
    int cur_line = 1;
    const char *cur_file = NULL;
    bool first_on_line = true;
    char *prev_sp = NULL;
    int prev_sl = 0;
    Token *prev_tok = NULL;
    for (; tok && tok->kind != TK_EOF; tok = tok->next) {
        int ln = tok->lineno > 0 ? tok->lineno : cur_line;
        const char *fn = tok->filename;
        if (!fn || *fn == '<') {
            fn = cur_file ? cur_file : "<stdin>";
            ln = cur_line;
        }
        if (!cur_file || strcmp(fn, cur_file) != 0) {
            // A file boundary (returning from an #include, or moving into
            // one) always starts a fresh logical line, even when the new
            // file's line number isn't "greater" than the old one in any
            // way this function can compare — sp/prev_sl adjacency is only
            // meaningful within the same buffer. Without this, the last
            // token emitted from the old file (e.g. a macro's ".endm") and
            // the first token of the new one land on the same output line,
            // silently merging two unrelated GAS statements into one (seen
            // as ".endm .macro UNWIND_HINT ..." — the ".macro" line then
            // never matches as a directive at all).
            if (!first_on_line) {
                if (len + 2 > cap) {
                    cap *= 2;
                    buf = realloc(buf, cap);
                }
                buf[len++] = '\n';
                first_on_line = true;
                prev_sp = NULL;
            }
            cur_line = ln;
            cur_file = fn;
        }
        while (cur_line < ln) {
            if (len + 2 > cap) {
                cap *= 2;
                buf = realloc(buf, cap);
            }
            buf[len++] = '\n';
            cur_line++;
            first_on_line = true;
            prev_sp = NULL; // a newline always breaks adjacency
        }
        int sl;
        char *sp = tok_spelling(tok, &sl);
        // The C lexer splits assembly's dot-containing directive/section
        // names (".section", ".init.ramfs", "label:") into separate
        // punctuation + identifier tokens, same as it would "a.b.c" in
        // real C. Re-glue tokens with no separating space whenever they
        // were byte-adjacent (no whitespace at all) in whatever buffer
        // they were lexed from — true both for un-expanded source text
        // and for a macro body's own internal spacing — so ".", "section"
        // comes back as ".section" but "section", ".init" (genuinely
        // space-separated in the source) keeps its space. A macro-expanded
        // token's spelling can live in a completely different buffer than
        // whatever body token follows it (the substituted argument text vs.
        // the macro's own stored definition), so pointer adjacency alone
        // can't see those as touching even when the macro body had none —
        // no_space_after (set by subst_range() for exactly this case, e.g.
        // linkage.h's "name:" built from a macro parameter) overrides that.
        bool adjacent = (prev_sp && sp && prev_sp + prev_sl == sp) ||
            (prev_tok && prev_tok->no_space_after);
        if (!first_on_line && !adjacent) {
            if (len + 2 > cap) {
                cap *= 2;
                buf = realloc(buf, cap);
            }
            buf[len++] = ' ';
        }
        if (sp && sl > 0) {
            if (len + (size_t)sl + 2 > cap) {
                while (len + (size_t)sl + 2 > cap) cap *= 2;
                buf = realloc(buf, cap);
            }
            memcpy(buf + len, sp, (size_t)sl);
            len += (size_t)sl;
        }
        first_on_line = false;
        prev_sp = sp;
        prev_sl = sl;
        prev_tok = tok;
    }
    if (len + 2 > cap) {
        cap *= 2;
        buf = realloc(buf, cap);
    }
    buf[len++] = '\n';
    buf[len] = '\0';
    return buf;
}

void pp_print_tokens(Token *tok) {
    int cur_line = 1;
    const char *cur_file = NULL;
    for (; tok && tok->kind != TK_EOF; tok = tok->next) {
        int ln = tok->lineno > 0 ? tok->lineno : cur_line;
        const char *fn = tok->filename;
        if (!fn || *fn == '<') {
            fn = cur_file ? cur_file : "<stdin>";
            ln = cur_line;
        }
        if (!cur_file || strcmp(fn, cur_file) != 0) {
            printf("# %d \"%s\"\n", ln, fn);
            cur_line = ln;
            cur_file = fn;
        }
        while (cur_line < ln) {
            putchar('\n');
            cur_line++;
        }
        int sl;
        char *sp = tok_spelling(tok, &sl);
        if (sp && sl > 0) fwrite(sp, 1, sl, stdout);
        putchar(' ');
    }
    putchar('\n');
}
char *dump_macros_text(void) {
    size_t total = 0;
    for (Macro *m = macros; m; m = m->next) {
        total += strlen(m->name) + 8;
        if (m->is_function) {
            total += 2;
            for (int i = 0; i < m->param_len; i++) {
                if (i > 0) total += 2;
                total += strlen(m->params[i]);
            }
            total += 1;
        }
        total += 1;
        for (Token *b = m->body; b; b = b->next) {
            int sl;
            tok_spelling(b, &sl);
            total += sl + 1;
        }
        total += 1;
    }
    char *buf = arena_alloc(total + 1);
    int n = 0;
    for (Macro *m = macros; m; m = m->next) {
        n += sprintf(buf + n, "#define %s", m->name);
        if (m->is_function) {
            buf[n++] = '(';
            for (int i = 0; i < m->param_len; i++) {
                if (i > 0) {
                    buf[n++] = ',';
                    buf[n++] = ' ';
                }
                int pl = strlen(m->params[i]);
                memcpy(buf + n, m->params[i], pl);
                n += pl;
            }
            if (m->is_variadic) {
                if (!m->is_gnu_variadic) buf[n++] = ',';
                buf[n++] = '.';
                buf[n++] = '.';
                buf[n++] = '.';
            }
            buf[n++] = ')';
        }
        buf[n++] = ' ';
        for (Token *b = m->body; b; b = b->next) {
            int sl;
            char *sp = tok_spelling(b, &sl);
            memcpy(buf + n, sp, sl);
            n += sl;
            if (b->next) buf[n++] = ' ';
        }
        buf[n++] = '\n';
    }
    buf[n] = '\0';
    return buf;
}
void print_search_dirs(const char *gcc) {
    printf("install: %s\n", RCC_INCDIR);
    for (int i = 0; i < nb_user_include_paths; i++) printf("include: =%s\n", user_include_paths[i]);
    for (int i = 0; sys_include_paths[i]; i++) printf("include: =%s\n", sys_include_paths[i]);
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s -print-search-dirs 2>/dev/null", gcc ? gcc : "gcc");
    FILE *fp = popen(cmd, "r");
    if (fp) {
        char line[4096];
        while (fgets(line, sizeof(line), fp))
            if (!strncmp(line, "libraries:", 10)) fputs(line, stdout);
        pclose(fp);
    }
}

// ============================================================
// Top-level preprocess()
// ============================================================

Token *preprocess(char *filename, char *p) {
    clear_macros();
    static char *builtin_expect_params[] = {"x", "y"};
    if (!macros_inited) {
#define define_pre(name, value) define_macro(name, false, NULL, 0, value)
        define_pre("__has_include", "1");
        define_pre("__has_c_attribute", "1");
        define_pre("__has_include_next", "1");
#include "gcc_predefined.h"
        if (opt_std_version) define_pre("__STDC_VERSION__", (char *)opt_std_version);
        if (!find_macro("__STDC_FENV_ACCESS__")) define_pre("__STDC_FENV_ACCESS__", "1");
        if (opt_std_version && strcmp(opt_std_version, "202311L") == 0) {
            if (!find_macro("bool")) define_pre("bool", "_Bool");
            if (!find_macro("__bool_true_false_are_defined")) define_pre("__bool_true_false_are_defined", "1");
        }
        if (opt_O1) define_pre("__OPTIMIZE__", "1");
#ifdef __APPLE__
        if (!find_macro("__APPLE__")) define_macro("__APPLE__", false, NULL, 0, "1");
        if (!find_macro("__leading_underscore")) define_macro("__leading_underscore", false, NULL, 0, "1");
        if (!find_macro("__MACH__")) define_macro("__MACH__", false, NULL, 0, "1");
#endif
#ifdef _WIN32
        if (!find_macro("__LLP64__")) define_pre("__LLP64__", "1");
#else
        if (!find_macro("__LP64__")) define_pre("__LP64__", "1");
#endif
        define_pre("__INT128_TYPE__", "__int128");
        define_pre("__UINT128_TYPE__", "unsigned __int128");
        define_pre("__uint128_t", "unsigned __int128");
        define_pre("__int128_t", "__int128");
        define_macro("__builtin_expect", true, builtin_expect_params, 2, "((void)(y),(x))");
        define_pre("__builtin_memcpy", "memcpy");
        define_pre("__builtin_memcmp", "memcmp");
        define_pre("__builtin_memset", "memset");
        define_pre("__builtin_strlen", "strlen");
        define_pre("__builtin_strcpy", "strcpy");
        define_pre("__builtin_strcmp", "strcmp");
        define_pre("__builtin_abort", "abort");
        define_pre("__builtin_malloc", "malloc");
        define_pre("__builtin_calloc", "calloc");
        define_pre("__builtin_realloc", "realloc");
        define_pre("__builtin_free", "free");
        define_pre("__builtin_memmove", "memmove");
        define_pre("__builtin_strncpy", "strncpy");
        define_pre("__builtin_strncmp", "strncmp");
        define_pre("__builtin_strcat", "strcat");
        define_pre("__builtin_strncat", "strncat");
        define_pre("__builtin_strchr", "strchr");
        define_pre("__builtin_strrchr", "strrchr");
        define_pre("__builtin_strdup", "strdup");
        define_pre("__builtin_alloca", "alloca");
        define_pre("__builtin_exit", "exit");
        define_pre("__builtin_printf", "printf");
        define_pre("__builtin_puts", "puts");
        define_pre("__builtin_sprintf", "sprintf");
        define_macro("__builtin_assume_aligned", true, (char *[]){"__p", "__a"}, 2, "(__p)");
        {
            char *p4[] = {"__dest", "__src", "__len", "__bos", NULL}, *p4m[] = {"__dest", "__src", "__len", "__bos", NULL};
            define_macro("__builtin___memcpy_chk", true, p4m, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),(__dest)):__builtin_memcpy(__dest,__src,__len))");
            define_macro("__builtin___memmove_chk", true, p4m, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),(__dest)):__builtin_memmove(__dest,__src,__len))");
            define_macro("__builtin___memset_chk", true, p4, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),(__dest)):__builtin_memset(__dest,__src,__len))");
            define_macro("__builtin___memcmp_chk", true, p4, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),0):__builtin_memcmp(__dest,__src,__len))");
            char *p3s[] = {"__dest", "__src", "__bos", NULL};
            define_macro("__builtin___strcpy_chk", true, p3s, 3, "((__bos)!=(unsigned long long)-1&&(__bos)<(__builtin_strlen(__src)+1)?(abort(),(__dest)):__builtin_strcpy(__dest,__src))");
            define_macro("__builtin___strncpy_chk", true, p4, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),(__dest)):__builtin_strncpy(__dest,__src,__len))");
            define_macro("__builtin___strcat_chk", true, p3s, 3, "((__bos)!=(unsigned long long)-1&&(__bos)<(__builtin_strlen(__dest)+__builtin_strlen(__src)+1)?(abort(),(__dest)):__builtin_strcat(__dest,__src))");
            define_macro("__builtin___strncat_chk", true, p4, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__builtin_strlen(__dest)+(__len)+1)?(abort(),(__dest)):__builtin_strncat(__dest,__src,__len))");
            char *p2[] = {"__s", "__bos", NULL};
            define_macro("__builtin___strlen_chk", true, p2, 2, "((__bos)!=(unsigned long long)-1&&(__bos)<(__builtin_strlen(__s)+1)?(abort(),0):__builtin_strlen(__s))");
            char *p2f[] = {"__fmt", "__bos", NULL};
            define_macro("__builtin___printf_chk", true, p2f, 2, "__builtin_printf");
            define_macro("__builtin___fprintf_chk", true, p2f, 2, "__builtin_fprintf");
            define_macro("__builtin___vfprintf_chk", true, p2f, 2, "__builtin_vfprintf");
            char *p3f[] = {"__dest", "__fmt", "__bos", NULL};
            define_macro("__builtin___sprintf_chk", true, p3f, 3, "((__bos)!=(unsigned long long)-1?(abort(),0):__builtin_sprintf(__dest,__fmt))");
            define_macro("__builtin___vsprintf_chk", true, p3f, 3, "((__bos)!=(unsigned long long)-1?(abort(),0):__builtin_vsprintf(__dest,__fmt))");
            char *p4f[] = {"__dest", "__len", "__fmt", "__bos", NULL};
            define_macro("__builtin___snprintf_chk", true, p4f, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),0):__builtin_snprintf(__dest,__len,__fmt))");
            define_macro("__builtin___vsnprintf_chk", true, p4f, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),0):__builtin_vsnprintf(__dest,__len,__fmt))");
            define_pre("__builtin___read_chk", "read");
            define_pre("__builtin___pread_chk", "pread");
            define_pre("__builtin___readlink_chk", "readlink");
            define_pre("__builtin___readlinkat_chk", "readlinkat");
            define_pre("__builtin___getcwd_chk", "getcwd");
            define_pre("__builtin___getwd_chk", "getwd");
            define_pre("__builtin___confstr_chk", "confstr");
            define_pre("__builtin___getgroups_chk", "getgroups");
            define_pre("__builtin___ttyname_r_chk", "ttyname_r");
            define_pre("__builtin___getlogin_r_chk", "getlogin_r");
            define_pre("__builtin___gethostname_chk", "gethostname");
            define_pre("__builtin___getdomainname_chk", "getdomainname");
            define_pre("__read_chk_warn", "read");
            define_pre("__pread_chk_warn", "pread");
            define_pre("__readlink_chk_warn", "readlink");
            define_pre("__readlinkat_chk_warn", "readlinkat");
            define_pre("__getcwd_chk_warn", "getcwd");
            define_pre("__getwd_warn", "getcwd");
            define_pre("__confstr_chk_warn", "confstr");
            define_pre("__getgroups_chk_warn", "getgroups");
            define_pre("__ttyname_r_chk_warn", "ttyname_r");
            define_pre("__getlogin_r_chk_warn", "getlogin_r");
            define_pre("__gethostname_chk_warn", "gethostname");
            define_pre("__getdomainname_chk_warn", "getdomainname");
            define_pre("__builtin___read_chk_warn", "read");
            define_pre("__builtin___pread_chk_warn", "pread");
            define_pre("__builtin___readlink_chk_warn", "readlink");
            define_pre("__builtin___readlinkat_chk_warn", "readlinkat");
            define_pre("__builtin___getcwd_chk_warn", "getcwd");
            define_pre("__builtin___getwd_warn", "getcwd");
            define_pre("__builtin___confstr_chk_warn", "confstr");
            define_pre("__builtin___getgroups_chk_warn", "getgroups");
            define_pre("__builtin___ttyname_r_chk_warn", "ttyname_r");
            define_pre("__builtin___getlogin_r_chk_warn", "getlogin_r");
            define_pre("__builtin___gethostname_chk_warn", "gethostname");
            define_pre("__builtin___getdomainname_chk_warn", "getdomainname");
        }
        define_pre("signbit", "__builtin_signbit");
        define_pre("__builtin_trap", "abort");
        define_macro("__builtin_clear_padding", true, (char *[]){"ptr"}, 1, "__builtin_memset(ptr, 0, sizeof(*(ptr)))");
#ifdef _WIN32
        define_pre("isinf", "__builtin_isinf");
        define_pre("isinff", "__builtin_isinff");
        define_pre("isinfl", "__builtin_isinfl");
#endif
        define_pre("__builtin_isnan", "isnan");
        define_pre("__builtin_isnanf", "isnan");
        define_pre("__builtin_isnanl", "isnan");
        define_pre("__builtin_snprintf", "snprintf");
        define_pre("__builtin_fprintf", "fprintf");
        define_pre("__builtin_vprintf", "vprintf");
        define_pre("__builtin_vsprintf", "vsprintf");
        define_pre("__builtin_vsnprintf", "vsnprintf");
        define_pre("__extension__", "");
#undef define_pre
        kw_line = str_intern("__LINE__", 8);
        kw_file = str_intern("__FILE__", 8);
        kw_counter = str_intern("__COUNTER__", 11);
        kw_function = str_intern("__FUNCTION__", 12);
        kw_func = str_intern("__func__", 8);
        kw_pretty_function = str_intern("__PRETTY_FUNCTION__", 19);
        kw_has_include = str_intern("__has_include", 13);
        kw_has_include_next = str_intern("__has_include_next", 18);
        kw_has_c_attribute = str_intern("__has_c_attribute", 17);
        kw_va_args = str_intern("__VA_ARGS__", 11);
        kw_va_opt = str_intern("__VA_OPT__", 10);
        kw_defined = str_intern("defined", 7);
        kw_true = str_intern("true", 4);
        kw_false = str_intern("false", 5);
        dn_define = str_intern("define", 6);
        dn_undef = str_intern("undef", 5);
        dn_include = str_intern("include", 7);
        dn_line = str_intern("line", 4);
        dn_error = str_intern("error", 5);
        dn_warning = str_intern("warning", 7);
        dn_if = str_intern("if", 2);
        dn_ifdef = str_intern("ifdef", 5);
        dn_ifndef = str_intern("ifndef", 6);
        dn_elif = str_intern("elif", 4);
        dn_elifdef = str_intern("elifdef", 7);
        dn_elifndef = str_intern("elifndef", 8);
        dn_else = str_intern("else", 4);
        dn_endif = str_intern("endif", 5);
        dn_pragma = str_intern("pragma", 6);
        saved_macros = macros;
        macros_inited = true;
    }

    frames = NULL;
    ungot = NULL;
    nframes = 0;
    pp_counter = 0;
    inc_depth = 0;
    xp_in_cond = false;

    char *resolved_name = (filename && strcmp(filename, "-") == 0) ? "<stdin>" : canonical_path(filename);
    lvl = NULL;
    // push_level() splices line continuations itself; passing raw input avoids a
    // double splice that would discard the physical-line counts (breaks __LINE__).
    // Push main source first, then pre-include files on top (LIFO).
    // Pre-includes must process first so their macros are visible.
    push_level(resolved_name, full_path(resolved_name), p);
    for (int i = nb_preinclude - 1; i >= 0; i--) {
        char *inc_path = full_path((char *)preinclude_list[i]);
        char *inc_contents = read_pp_file((char *)preinclude_list[i]);
        if (inc_contents) {
            push_level(inc_path, inc_path, inc_contents);
            dep_add(inc_path);
        }
    }
    lex_pp_mode = true;
    xout_head = xout_tail = NULL;

    for (;;) {
        Token *t = ungot_pull();
        if (!t) t = pp_next_raw();
        if (t == &mark_eof) break;
        if (t == &mark_directive) {
            do_directive();
            continue;
        }
        // Track the current source invocation site so __LINE__/__FILE__ inside a
        // macro body report where the macro is used, not where it was defined.
        pp_cur_line = t->lineno;
        pp_cur_file = t->filename;
        expand_token(t);
        drain_frames();
    }
    Token *eof = new_pp_token(TK_EOF, NULL);
    eof->filename = lvl->filename;
    eof->lineno = lvl->reported_line;
    eof->ptr = lvl->p;
    out_append(eof);

    lex_pp_mode = false;
    Token *result = concat_strings(xout_head);
    if (opt_dM) return NULL;
    return result;
}

// Write Make dependency rules (-Wp,-MMD,<file>)
void write_dep_file(const char *out_path, const char *main_fpath) {
    if (!opt_depfile || !main_fpath) return;
    FILE *f = fopen(opt_depfile, "w");
    if (!f) {
        fprintf(stderr, "rcc: error: cannot open dependency file '%s'\n", opt_depfile);
        return;
    }
    // Target: output file depends on main source + all included files.
    // Input read from stdin ("-") isn't a real path on disk — GCC omits it
    // from the dependency list rather than emitting an unopenable "-"
    // entry (which trips up kbuild's fixdep, e.g. scripts/checksyscalls.sh
    // piping a generated source through `$(CC) ... -x c -`).
    fprintf(f, "%s:", out_path ? out_path : "a.out");
    if (strcmp(main_fpath, "-") != 0)
        fprintf(f, " %s", main_fpath);
    for (DepEntry *d = dep_files; d; d = d->next) {
        if (d->path) fprintf(f, " %s", d->path);
    }
    fprintf(f, "\n");
    fclose(f);
}
