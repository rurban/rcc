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
#include <time.h>
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
    char **params;
    int param_len;
    Token *body;
    unsigned hh_mask;
};

#define MACRO_HT_SIZE 2048
static Macro *macro_htab[MACRO_HT_SIZE];

static char *kw_line;
static char *kw_file;
static char *kw_base_file;
static char *kw_counter;
static char *kw_function;
static char *kw_func;
static char *kw_pretty_function;
static char *kw_date;
static char *kw_time;
static char *kw_has_include;
static char *kw_has_include_next;
static char *kw_has_c_attribute;
static char *kw_has_builtin;
static char *kw_va_args;
static char *kw_va_opt;
static char *kw_defined;
static char *kw_true;
static char *kw_false;

static char *dn_define, *dn_undef, *dn_include, *dn_include_next, *dn_line, *dn_error, *dn_warning;
static char *dn_if, *dn_ifdef, *dn_ifndef, *dn_elif, *dn_elifdef, *dn_elifndef;
static char *dn_else, *dn_endif, *dn_pragma, *dn_embed;

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
// __BASE_FILE__: the main input file's name, unlike __FILE__/pp_cur_file
// which tracks whichever file is *currently* being read (changes across
// #include). Set once per preprocess() call, from the top-level source,
// never touched again — including while inside a pre-include file pushed
// on top of it (see preprocess()'s push_level() call order).
static char *pp_base_file;
static Macro *cmdline_macros;
static Macro *saved_macros;
static MacroStack *macro_stack;
static const char *user_include_paths[64];
static int nb_user_include_paths;
// -iquote dirs: GCC searches these ONLY for `#include "..."`, never for
// `#include <...>` -- unlike -I/-isystem/-idirafter, which apply to both
// forms and stay in user_include_paths above. Kept in a separate list so
// build_search_dirs() can skip it for angle includes (see is_angle
// param). Confirmed against real gcc: `-iquote dir` with `dir/foo.h`
// present must NOT satisfy `#include <foo.h>`.
static const char *quote_include_paths[64];
static int nb_quote_include_paths;
static bool macros_inited;

void add_include_path(const char *path) {
    if (nb_user_include_paths < 64)
        user_include_paths[nb_user_include_paths++] = str_intern(path, strlen(path));
}

void add_quote_include_path(const char *path) {
    if (nb_quote_include_paths < 64)
        quote_include_paths[nb_quote_include_paths++] = str_intern(path, strlen(path));
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
    nb_quote_include_paths = 0;
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
                if (idx < 32 && is_hashhash) mask |= 1u << idx;
                if (idx < 32 && was_hashhash) mask |= 1u << idx;
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
// Same as define_macro(), but for a variadic function-like macro: params
// lists only the named leading parameters, and the body may reference
// __VA_ARGS__ for whatever trailing arguments the call site supplies.
// Needed because define_macro() unconditionally passes is_variadic=false
// to define_macro_tok() — a call whose actual argument count exceeds a
// non-variadic macro's declared param count is left completely
// unexpanded (see the _FORTIFY_SOURCE sprintf-family macros below, whose
// param counts didn't match glibc's real chk-function signatures).
static void define_macro_va(char *name, char **params, int param_len, char *body) {
    name = str_intern(name, strlen(name));
    char **pc = NULL;
    if (param_len > 0) {
        pc = arena_alloc(sizeof(char *) * param_len);
        for (int i = 0; i < param_len; i++) pc[i] = str_intern(params[i], strlen(params[i]));
    }
    Token *btoks = lex_body_string(body, "<builtin>", 1);
    define_macro_tok(name, true, pc, param_len, btoks, true, false);
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

#ifndef RCC_INCDIR
#define RCC_INCDIR "include"
#endif

// Build the ordered list of directories searched for an include, matching
// real GCC's actual precedence: (quote form only) -iquote dirs, then
// user -I/-isystem/-idirafter paths, then rcc's own bundled include dir
// (RCC_INCDIR / its "include" source-tree fallback -- rcc's analogue of
// GCC's own private GCC_INCLUDE_DIR, e.g. .../gcc/16/include, which
// likewise sits AFTER -I in GCC's real search order), then system paths.
// Confirmed directly against real gcc for both forms: a -I (or -iquote,
// for quotes) directory providing its own stddef.h/string.h/etc. is
// found ahead of gcc's own bundled copy of the same name.
//
// This used to keep RCC_INCDIR ahead of every -I directory for the
// angle form specifically (differing from -iquote, fixed earlier for
// quotes only), on the theory that ast/ksh93's own relative-escape
// `#include <../include/wchar.h>` idiom needed RCC_INCDIR resolved
// first. It doesn't: that idiom is driven entirely by
// resolve_include()'s own "skip a match already active on the include
// stack" guard (the self_active check below, keyed off RCC_INCDIR's
// [bundled_lo, bundled_hi) range regardless of that range's position in
// this list) plus resolve_include_next()'s is_noop_forward_to_active()
// skip -- neither depends on RCC_INCDIR coming before -I. Confirmed
// against ksh93's own third_party build after this reorder: unchanged,
// still fully passing.
//
// Keeping RCC_INCDIR ahead of -I, however, broke every project shipping
// its own gnulib-style "-I override + #include_next onward" replacement
// header sharing a name with something rcc bundles (stddef.h, stdint.h,
// limits.h, stdbool.h, float.h, stdarg.h -- rcc's OWN bundled copies of
// these are fully self-contained, with no #include_next of their own to
// chain through to such an override): findutils' gl/lib/stddef.h
// (providing gl_unreachable(), and relying on being the FIRST responder
// to *every* `#include <stddef.h>` in the TU -- including deep,
// __need_size_t-restricted requests from glibc's own headers like
// bits/types/struct_iovec.h -- to correctly track and clean up the
// __need_XXX protocol state itself) was silently unreachable, an
// undiagnosed dead end: "undefined reference to `gl_unreachable'" only
// at link time, with no compile-time signal at all.
static int build_search_dirs(const char **dirs, int max, bool is_angle, int *bundled_lo, int *bundled_hi) {
    int n = 0;
    if (!is_angle)
        for (int i = 0; i < nb_quote_include_paths && n < max; i++)
            dirs[n++] = quote_include_paths[i];
    for (int i = 0; i < nb_user_include_paths && n < max; i++)
        dirs[n++] = user_include_paths[i];
    // RCC_INCDIR and its "include" source-tree fallback occupy a known,
    // fixed range right here -- tracked positionally (not by comparing
    // dirs[i]'s string value) so a user -I/project include dir that
    // happens to also be spelled "include" (extremely common convention:
    // chibi-scheme, many others) is never mistaken for rcc's own bundled
    // headers below.
    int lo = n;
    if (n < max) dirs[n++] = RCC_INCDIR;
    if (strcmp(RCC_INCDIR, "include") != 0 && n < max) dirs[n++] = "include";
    if (bundled_lo) *bundled_lo = lo;
    if (bundled_hi) *bundled_hi = n;
    if (!opt_nostdinc)
        for (int i = 0; sys_include_paths[i] && n < max; i++)
            dirs[n++] = sys_include_paths[i];
    return n;
}

// `curr_file`/`curr_display` are the including file's absolute-identity
// path and its as-given display name respectively (see push_level()'s
// display/fpath split). The return value is always the absolute path
// used to actually open/track the file. `out_display`, when non-NULL,
// additionally receives a "nice" name for #line markers: real cpp shows
// a quoted include resolved next to the including file using a path
// relative to how *that* file was itself named (e.g. util.c including
// "vutil.c" shows as bare "vutil.c", not an absolute path) — GCC never
// absolutizes same-directory quoted includes. rcc used to always return
// (and display) the fully canonicalized absolute path here, which is
// harmless for compilation but corrupts `makedepend`-style dependency
// scanners: Perl's makedependfile.SH greps the preprocessor's own #line
// output and treats any dependency path containing '/' as "this .c is
// independently compilable", so an absolute "/…/vutil.c" marker made it
// wrongly attach a standalone build recipe to vutil.c — a file that is
// only ever #include-d into util.c and cannot be compiled on its own.
// Left NULL for angle includes and search-path/system hits, which really
// do warrant an absolute display name (matching gcc there too).
static char *resolve_include_raw(char *curr_file, char *curr_display, char *spec, bool is_angle,
                                 char **out_display) {
    char *path;
    if (!is_angle) {
        char *dir = path_dirname(curr_file);
        path = path_join(dir, spec);
        if (file_exists(path)) {
            if (out_display)
                *out_display = path_join(path_dirname(curr_display), spec);
            return canonical_path(path);
        }
    }
    const char *dirs[128];
    int nd = build_search_dirs(dirs, 128, is_angle, NULL, NULL);
    for (int i = 0; i < nd; i++) {
        path = path_join(dirs[i], spec);
        if (file_exists(path)) return canonical_path(path);
    }
    // Angle-bracket includes never implicitly search the current working
    // directory (only explicit -I/-iquote dirs and the built-in system
    // list above) -- only a quote include, which C already lets fall
    // back to a plain relative-to-cwd lookup when curr_file's own
    // directory search (above) didn't find it, reaches this fallback.
    // Without this guard, `#include <name.h>` could silently resolve to
    // an unrelated same-named file sitting in the compiler's cwd even
    // though no search directory (-I or system) actually provides it --
    // confirmed against real gcc, which never does this.
    if (!is_angle && file_exists(spec)) return canonical_path(spec);
    return NULL;
}

// A relative-escape spec (e.g. ast/ksh93's own `#include
// <../include/wchar.h>`, meant to reach the platform's *native* header)
// joined against RCC_INCDIR itself can collapse right back onto
// RCC_INCDIR's own bundled copy of the same name whenever RCC_INCDIR's
// own basename is "include" (`.../rcc/include/../include/X.h` lexically
// IS `.../rcc/include/X.h`) — defeating the whole point of the escape.
// If that bundled copy is *already* active on the include stack (we're
// mid-`#include_next` from inside it, which is exactly when this idiom
// is used), the real #include directive handler below must skip past
// it to the actual next candidate (a user -I directory or the real
// system header) instead of silently re-triggering the same file's own
// include guard — matching resolve_include_next()'s own no-op-forward
// handling for the same self-reference shape. is_noop_forward_to_active()
// below deliberately calls resolve_include_raw() instead of this wrapper
// for its own internal peek: it needs the *unfiltered* answer ("what
// would a bare #include from this file actually resolve to") to
// recognize that exact self-reference collision as its "already active"
// signal in the first place.
static char *resolve_include(char *curr_file, char *curr_display, char *spec, bool is_angle,
                             char **out_display, bool allow_cwd_fallback) {
    if (!is_angle) {
        char *dir = path_dirname(curr_file);
        char *path = path_join(dir, spec);
        if (file_exists(path)) {
            if (out_display)
                *out_display = path_join(path_dirname(curr_display), spec);
            return canonical_path(path);
        }
    }
    const char *dirs[128];
    int bundled_lo = 0, bundled_hi = 0;
    int nd = build_search_dirs(dirs, 128, is_angle, &bundled_lo, &bundled_hi);
    for (int i = 0; i < nd; i++) {
        char *path = path_join(dirs[i], spec);
        if (!file_exists(path)) continue;
        if (i >= bundled_lo && i < bundled_hi) {
            char *resolved = canonical_path(full_path(path));
            bool self_active = false;
            for (PPLvl *l = lvl; l; l = l->next)
                if (l->fpath && !strcmp(canonical_path(l->fpath), resolved)) {
                    self_active = true;
                    break;
                }
            if (self_active) continue;
        }
        return canonical_path(path);
    }
    // Neither angle-bracket nor quote-form #include/__has_include ever
    // implicitly searches the current working directory: quote includes
    // search the including file's own directory (handled above), then
    // -iquote/-I/system dirs (the loop above); angle includes skip
    // straight to -I/system dirs. Confirmed against real gcc for both
    // forms directly: `#include "foo.h"` sitting in cwd, but not in the
    // compiled file's own directory nor any -I dir, is a clean "No such
    // file or directory" -- gcc never falls back to a bare cwd lookup
    // either. #embed is a different construct with its own, separate
    // search-path mechanism in real GCC (--embed-dir=, entirely
    // unrelated to -I) that rcc doesn't implement; `allow_cwd_fallback`
    // (set only by #embed's own call site) intentionally preserves rcc's
    // existing, pragmatic cwd-relative resolution for #embed <file>
    // rather than making it stricter than real GCC in the OTHER
    // direction (#embed with no search path at all would never resolve
    // anything).
    if (allow_cwd_fallback && file_exists(spec)) return canonical_path(spec);
    return NULL;
}

// True if `path`'s entire content, once comments and blank lines are
// skipped, reduces to a single `#include <X>`/`#include "X"` whose
// resolved target is *already* active on the include stack (walking
// `lvl`) -- a trivial one-line forwarding wrapper (a common ast/ksh93
// idiom: e.g. its own std/wchar.h is just `#include <ast_wchar.h>`)
// that would silently contribute nothing back into the very chain
// #include_next is trying to escape, hiding whatever the real header
// underneath it provides. Anything more complex (a real guard,
// declarations of its own, ...) is left alone -- only this exact
// narrow "does nothing but forward to something already open" shape is
// special-cased, so resolve_include_next() below can skip past it and
// keep searching instead of settling for a no-op match.
static bool is_noop_forward_to_active(char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return false;
    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    buf[n] = '\0';
    char *p = buf;
    for (;;) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (p[0] == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/')) p++;
            if (*p) p += 2;
            continue;
        }
        if (p[0] == '/' && p[1] == '/') {
            while (*p && *p != '\n') p++;
            continue;
        }
        break;
    }
    if (*p != '#') return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "include", 7) != 0) return false;
    p += 7;
    if (*p != ' ' && *p != '\t' && *p != '<' && *p != '"') return false;
    while (*p == ' ' || *p == '\t') p++;
    char close;
    bool angle;
    if (*p == '<') {
        close = '>';
        angle = true;
    } else if (*p == '"') {
        close = '"';
        angle = false;
    } else
        return false;
    p++;
    char *start = p;
    while (*p && *p != close && *p != '\n') p++;
    if (*p != close) return false;
    char *target = pp_strndup(start, p - start);
    char *resolved = resolve_include_raw(path, path, target, angle, NULL);
    if (!resolved) return false;
    resolved = canonical_path(full_path(resolved));
    for (PPLvl *l = lvl; l; l = l->next)
        if (l->fpath && !strcmp(canonical_path(l->fpath), resolved))
            return true;
    return false;
}

// #include_next: search the same ordered list, but start *after* the directory
// that supplied curr_file. Lets a bundled header (e.g. include/wchar.h) fall
// through to the system header of the same name.
static char *resolve_include_next(char *curr_file, char *spec, bool is_angle) {
    const char *dirs[128];
    int bundled_lo = 0, bundled_hi = 0;
    int nd = build_search_dirs(dirs, 128, is_angle, &bundled_lo, &bundled_hi);
    // curr_file resolved from a spec with a subdirectory component (e.g.
    // <sys/stat.h>) lives at "<searchdir>/sys/stat.h": its directory is
    // "<searchdir>/sys", not the search-path entry "<searchdir>" itself.
    // Strip the spec's own subdirectory suffix first so the match below
    // compares against the base search directory that was actually used
    // -- otherwise no `dirs[]` entry ever matches, `start` stays 0, and the
    // search restarts from the top and re-finds this very file, recursing
    // through #include_next until the depth limit trips.
    char *file_dir = path_dirname(curr_file);
    char *spec_dir = path_dirname(spec);
    size_t file_dir_len = strlen(file_dir);
    size_t spec_dir_len = strlen(spec_dir);
    if (spec_dir_len > 0 && spec_dir_len < file_dir_len &&
        !strcmp(file_dir + file_dir_len - spec_dir_len, spec_dir)) {
        file_dir[file_dir_len - spec_dir_len] = '\0';
    }
    char *cur_dir = canonical_path(full_path(file_dir));
    // RCC_INCDIR and its "include" source-tree fallback occupy the fixed
    // [bundled_lo, bundled_hi) range from build_search_dirs() -- two
    // alternate *physical* locations for the SAME logical bundled-header
    // set: an installed copy (e.g. /usr/local/include/rcc, which every
    // build -- installed or not -- defaults RCC_INCDIR to) and the source
    // tree's own include/. A dev/test invocation of a non-installed
    // binary on a machine that has ever run `make install` has BOTH
    // present with byte-identical content. If the current file was found
    // in *either* slot, #include_next must escape the whole bundled-
    // header rung, not just the one physical path that matched:
    // otherwise the *other* slot's identical copy is "found" next, its
    // include guard (already set by the first copy) silently swallows
    // its entire body -- including its own #include_next -- so the real
    // system header underneath is never reached and #include_next
    // resolves to a file that contributes no content at all.
    // The dir that actually supplied curr_file is the FIRST dirs[]
    // entry matching cur_dir (resolve_include() returns the first
    // match). A later duplicate of the same physical dir (e.g.
    // /usr/include listed both as a -I dir and in sys_include_paths)
    // must NOT override start, or everything between the two entries
    // -- RCC_INCDIR's bundled copy of the header -- is skipped and
    // #include_next reports "not found" (INT_MAX et al. never defined
    // for -I/usr/include TUs).
    int start = 0;
    for (int i = 0; i < nd; i++) {
        if (!strcmp(cur_dir, canonical_path(full_path((char *)dirs[i])))) {
            start = i + 1;
            if (i >= bundled_lo && i < bundled_hi)
                start = bundled_hi;
            break;
        }
    }
    for (int i = start; i < nd; i++) {
        char *path = path_join(dirs[i], spec);
        // A user -I directory may legitimately provide its own
        // replacement for a bundled header (e.g. ksh93's own std/stdio.h
        // forwarding to its sfio-based ast_stdio.h) -- only a trivial
        // forward back into a header already open is a no-op to skip.
        if (file_exists(path) && !is_noop_forward_to_active(path))
            return canonical_path(path);
    }
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
    // codeql[cpp/loop-variable-changed]: deliberate i++ to also consume the '\\n' continuation's newline as one spliced unit
    for (int i = 0; i < len; i++) {
        // A line-continuation backslash immediately followed by a CRLF
        // pair (not just bare LF) must splice too -- a CRLF-terminated
        // source file (common on Windows-authored third-party sources,
        // e.g. unqlite.c) left the '\r' sitting between the backslash and
        // the '\n' this check alone was matching, so it silently never
        // fired: the backslash stayed as literal text right before the
        // physical newline, terminating the #define after only its first
        // line and leaving the rest of the macro body to be mis-lexed as
        // fresh top-level C ("type defaults to int" / "expected specific
        // operator" on the orphaned `ptr->member = ...;` continuation
        // lines). A stray '\r' immediately before an ordinary (non-
        // continuation) '\n' elsewhere is harmless -- it falls through to
        // the plain copy below and the lexer already treats it as
        // whitespace -- so only the continuation match itself needs
        // widening, not a general CRLF-to-LF normalization pass.
        if (input[i] == '\\' && input[i + 1] == '\r' && input[i + 2] == '\n') {
            i += 2;
            count++;
        } else if (input[i] == '\\' && input[i + 1] == '\n') {
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
// True if `path` is already absolute (POSIX: leading '/'; Windows: also a
// drive letter + ':', or a leading '\\') -- an absolute path ignores
// whatever base directory it's being joined against, same as every other
// path-joining utility (os.path.join, std::filesystem::path::append, ...).
static bool is_absolute_path(const char *path) {
    if (!path || !*path) return false;
#ifdef _WIN32
    if (path[0] == '/' || path[0] == '\\') return true;
    if (((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) &&
        path[1] == ':')
        return true;
    return false;
#else
    return path[0] == '/';
#endif
}
static char *path_join(const char *dir, const char *file) {
    if (!*dir || is_absolute_path(file)) return str_intern(file, strlen(file));
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
    // Read the whole file with a growing buffer. A fixed 10 MiB cap used to
    // silently truncate anything larger (fread() short-read, no EOF check)
    // -- a generated header just over the cap (e.g. lexbor's 12.2 MiB
    // unicode_normalization_test_res.h) then had its final declarations cut
    // mid-token, leaving an unbalanced construct the parser reported as a
    // bogus "expected specific operator" at end of input. Real compilers
    // handle arbitrarily large translation units; so do we now.
    size_t cap = 1 << 20, size = 0;
    char *buf = malloc(cap);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    for (;;) {
        if (size + 1 >= cap) {
            cap *= 2;
            char *nb = realloc(buf, cap);
            if (!nb) {
                free(buf);
                fclose(fp);
                return NULL;
            }
            buf = nb;
        }
        size_t n = fread(buf + size, 1, cap - size - 1, fp);
        if (n == 0) break;
        size += n;
    }
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
    // Hide set (C99 6.10.3.4p2) of this expansion: the invocation token's
    // own set plus the macro name; every pulled replacement token is
    // unioned with it (see frame_pull()).
    struct Hideset *hide;
    // Location of the macro invocation. Tokens produced by the expansion are
    // restamped with it so diagnostics (and __LINE__) point at the use site,
    // not the macro definition. stamp==false leaves token locations untouched.
    bool stamp;
    char *exp_file;
    int exp_line;
    // True when the object-like macro invocation this frame expands had no
    // source whitespace before the token that follows it in the enclosing
    // scan (see expand_token()'s object-like branch). frame_pull() stamps
    // this onto the copy of the frame's *last* body token so a later
    // #-stringize sees the expansion's result as tight against whatever
    // comes next, matching the adjacency of the pre-expansion macro name -
    // not the body token's own unrelated position in the #define line.
    bool tight_after;
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
static bool hs_contains(struct Hideset *h, Macro *m) {
    for (; h; h = h->next)
        if (h->name == m) return true;
    return false;
}
// Union: return a set containing h's names plus m (m added iff absent).
static struct Hideset *hs_add(struct Hideset *h, Macro *m) {
    if (!m || hs_contains(h, m)) return h;
    struct Hideset *n = arena_alloc(sizeof(struct Hideset));
    n->name = m;
    n->next = h;
    return n;
}
static void push_frame(Token *list, Macro *mac) {
    Frame *f = arena_alloc(sizeof(Frame));
    f->pos = list;
    f->mac = mac;
    f->stamp = false;
    f->hide = NULL;
    f->next = frames;
    frames = f;
    nframes++;
}
// Push a macro expansion whose tokens should be restamped to the invocation site.
// The frame's hide set = the invocation token's own hide set plus the macro
// itself (C99 6.10.3.4p2): every token pulled from the replacement list is
// marked with the whole union, so a recursion cycle that re-enters any
// ancestor macro stops as soon as the token's own name is already painted.
static void push_expansion(Token *list, Macro *mac, Token *site) {
    push_frame(list, mac);
    if (site) {
        frames->stamp = true;
        frames->exp_file = site->filename;
        frames->exp_line = site->lineno;
        frames->hide = hs_add(site->blue, mac);
    } else if (mac) {
        frames->hide = hs_add(NULL, mac);
    }
}
static bool str_needs_space(Token *a, Token *b);
static Token *frame_pull(void) {
    while (frames != frame_floor) {
        Frame *top = frames;
        Token *t = top->pos;
        if (!t || t->kind == TK_EOF) {
            frames = top->next;
            nframes--;
            continue;
        }
        top->pos = t->next;
        Token *c = copy_token(t);
        // C99 6.10.3.4p2 blue paint / hide set: tokens produced by a
        // macro's expansion are marked with the frame's hide set (the
        // invocation's own set plus the macro name), unioned with whatever
        // set the token already carried from its previous life (a
        // substituted argument keeps its own paints). The token is never
        // re-expanded as any name in the resulting set. ## paste results
        // (no_paint) are exempt: the paste happens fresh and the surrounding
        // macro's paint must not attach to the pasted spelling.
        if (top->hide && !c->no_paint) {
            for (struct Hideset *h = top->hide; h; h = h->next)
                c->blue = hs_add(c->blue, h->name);
        }
        if (top->stamp) {
            c->filename = top->exp_file;
            c->lineno = top->exp_line;
        }
        // Preserve "no source whitespace before the next sibling token"
        // across this pull: if `t` is itself about to be macro-expanded
        // (see expand_token()'s object-like branch), the expansion's own
        // buffer-pointer adjacency to whatever follows is lost - a plain
        // token comparison can no longer see it, so stash the fact here
        // while `t->next` still points at the real sibling.
        if (t->next && !str_needs_space(t, t->next))
            c->no_space_after = true;
        // If this is the frame's last body token and the frame itself was
        // pushed while its invocation was tight against what follows it,
        // that tightness carries onto this last token's own copy too.
        if (top->tight_after && (!t->next || t->next->kind == TK_EOF))
            c->no_space_after = true;
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
// #-stringize per C99 6.10.3.2: concatenate argument token spellings (one
// space where the source had whitespace between two tokens, none where
// they were adjacent), then wrap in quotes. The standard's escaping rule
// is narrow — "a \ character is inserted before each \" and \\ character
// of a character constant or string literal (including the delimiting \"
// characters)" — i.e. only backslash/quote bytes that are part of a
// *nested* string or char literal among the argument tokens get doubled.
// A bare '\' from an ordinary token must come through unescaped: GAS's
// own "\param" macro-parameter syntax is exactly this case (e.g.
// nospec-branch.h's __stringify(__FILL_RETURN_BUFFER(\reg,\nr)), used to
// build a run of instruction text as a macro argument) — real cpp
// stringifies it to "...\reg...\nr..." (single backslash), and the
// assembler's own macro-argument dequoting (strip_arg_quotes() in asm.c)
// only strips the surrounding quotes, it never undoes C-style escapes.
// Blanket-escaping every backslash here would silently corrupt every
// such GAS-macro-parameter reference into "\<value>" prefixed with a
// stray literal backslash once substituted into the macro body.
//
// `*out_raw`/`*out_rawlen` receive the token's semantic *decoded* value —
// the same content this function returned before this rule was applied —
// for the synthesized token's ->str/->len; the return value is the fully
// quoted *spelling* (starting with '"', ending with '"') for ->ptr/->val.
static char *stringize_build(Token *list, char **out_raw, int *out_rawlen) {
    int total = 3;
    for (Token *t = list; t && t->kind != TK_EOF; t = t->next) {
        int sl;
        tok_spelling(t, &sl);
        total += (t->kind == TK_STR) ? sl * 2 : sl;
        total += 1;
    }
    char *raw = arena_alloc(total);
    char *sp_out = arena_alloc(total);
    int rn = 0, sn = 0;
    sp_out[sn++] = '"';
    Token *prev = NULL;
    for (Token *t = list; t && t->kind != TK_EOF; t = t->next) {
        int sl;
        char *sp = tok_spelling(t, &sl);
        if (prev && str_needs_space(prev, t)) {
            raw[rn++] = ' ';
            sp_out[sn++] = ' ';
        }
        memcpy(raw + rn, sp, sl);
        rn += sl;
        if (t->kind == TK_STR) {
            for (int i = 0; i < sl; i++) {
                if (sp[i] == '"' || sp[i] == '\\') sp_out[sn++] = '\\';
                sp_out[sn++] = sp[i];
            }
        } else {
            memcpy(sp_out + sn, sp, sl);
            sn += sl;
        }
        prev = t;
    }
    raw[rn] = '\0';
    sp_out[sn++] = '"';
    sp_out[sn] = '\0';
    *out_raw = raw;
    *out_rawlen = rn;
    return sp_out;
}
static char *stringize_list(Token *list, char **out_raw, int *out_rawlen) {
    return stringize_build(list, out_raw, out_rawlen);
}
static char *stringize_va(Macro *m, Token **args, int argc, char **out_raw, int *out_rawlen) {
    int start = va_slot(m), total = 3;
    for (int i = start; i < argc; i++) {
        for (Token *t = args[i]; t && t->kind != TK_EOF; t = t->next) {
            int sl;
            tok_spelling(t, &sl);
            total += (t->kind == TK_STR) ? sl * 2 : sl;
            total += 1;
        }
        total += 1;
    }
    char *raw = arena_alloc(total);
    char *sp_out = arena_alloc(total);
    int rn = 0, sn = 0;
    sp_out[sn++] = '"';
    for (int i = start; i < argc; i++) {
        if (i > start) {
            raw[rn++] = ',';
            sp_out[sn++] = ',';
        }
        Token *prev = NULL;
        for (Token *t = args[i]; t && t->kind != TK_EOF; t = t->next) {
            int sl;
            char *sp = tok_spelling(t, &sl);
            if (prev && str_needs_space(prev, t)) {
                raw[rn++] = ' ';
                sp_out[sn++] = ' ';
            }
            memcpy(raw + rn, sp, sl);
            rn += sl;
            if (t->kind == TK_STR) {
                for (int k = 0; k < sl; k++) {
                    if (sp[k] == '"' || sp[k] == '\\') sp_out[sn++] = '\\';
                    sp_out[sn++] = sp[k];
                }
            } else {
                memcpy(sp_out + sn, sp, sl);
                sn += sl;
            }
            prev = t;
        }
    }
    raw[rn] = '\0';
    sp_out[sn++] = '"';
    sp_out[sn] = '\0';
    *out_raw = raw;
    *out_rawlen = rn;
    return sp_out;
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
    // codeql[cpp/loop-variable-changed]: deliberate b = n/c skip-ahead past the ##/#/__VA_OPT__ operand token(s) just consumed (5 sites below)
    for (Token *b = body; b && b != end && b->kind != TK_EOF; b = b->next) {
        int is_hashhash = b->kind == TK_PUNCT && b->len == 2 && b->ptr[0] == '#' && b->ptr[1] == '#';
        if (is_hashhash && b->next && b->next != end && b->next->kind != TK_EOF) {
            Token *n = b->next;
            // C23: ## __VA_OPT__(content) — evaluate __VA_OPT__ first.
            // If va_args is empty it expands to nothing and ## is a
            // placemarker (deleted).  If non-empty, the content is
            // substituted and ## pastes it with the preceding token.
            if (m->is_variadic && n->kind == TK_IDENT && n->name == kw_va_opt) {
                Token *o = n->next;
                if (o && o != end && ptok(o, "(")) {
                    int depth = 1;
                    Token *c = o->next;
                    while (c && c != end && c->kind != TK_EOF && depth > 0) {
                        if (ptok(c, "(")) depth++;
                        else if (ptok(c, ")"))
                            depth--;
                        if (depth > 0) c = c->next;
                    }
                    if (va_empty) {
                        // placemarker: ## deleted
                        b = c;
                        continue;
                    }
                    // Substitute __VA_OPT__ content and paste with lhs
                    Token *sub = subst_range(m, o->next, c, args, raw_args, argc);
                    if (!sub) {
                        b = c;
                        continue;
                    }
                    if (!rtail) {
                        rhead = sub;
                        while (sub->next) sub = sub->next;
                        rtail = sub;
                        b = c;
                        continue;
                    }
                    Token *lhs = pop_tail(&rhead, &rtail);
                    int l1, l2;
                    char *s1 = tok_spelling(lhs, &l1), *s2 = tok_spelling(sub, &l2);
                    char *pasted = arena_alloc(l1 + l2 + 1);
                    memcpy(pasted, s1, l1);
                    memcpy(pasted + l1, s2, l2);
                    pasted[l1 + l2] = '\0';
                    Token *pt = lex_body_string(pasted, lhs->filename, lhs->lineno);
                    pt->no_paint = true;
                    if (sub->next)
                        pt->no_space_after = sub->no_space_after;
                    splice_tokens(&rhead, &rtail, pt);
                    splice_tokens(&rhead, &rtail, sub->next);
                    b = c;
                    continue;
                }
            }
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
            pt->no_paint = true;
            // pt's spelling lives in a freshly lexed buffer, so the normal
            // pointer-adjacency check in str_needs_space() can never see it
            // as touching whatever follows — even when the macro body had
            // zero whitespace between the ## operator's rhs operand and the
            // next body token (e.g. "__export_symbol_##sym:"). Recover that
            // from the pre-substitution body tokens (n, n->next) so a later
            // #-stringize of this expansion keeps them adjacent too.
            //
            // When xhead (the ## rhs operand, post-argument-prescan)
            // expanded to *more than one* token — e.g. CONCATENATE(defs_,
            // _PT_FMT_H) where _PT_FMT_H is itself "PT_FMT.h" and PT_FMT
            // further expands to a single token, leaving xhead as
            // [x86_64, ., h] — the tightness that matters isn't `n`'s
            // (the pre-expansion "_PT_FMT_H" placeholder)'s own body
            // adjacency, it's whether xhead's *first* token was tight
            // against xhead->next in its own expansion. That's exactly
            // what xhead->no_space_after already records (set when PT_FMT
            // itself was expanded, via frame_pull()'s tight_after
            // propagation) — inherit it onto the pasted token instead of
            // defaulting to "needs a space" and splicing e.g. "defs_x86_64"
            // + " " + ".h" back together as a __stringify()'d filename.
            if (xhead->next)
                pt->no_space_after = xhead->no_space_after;
            else if (n->next && !str_needs_space(n, n->next))
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
                    char *raw;
                    int rawlen;
                    char *spelling = (m->is_variadic && idx == vs)
                        ? stringize_va(m, args, argc, &raw, &rawlen)
                        : (idx < argc ? stringize_list(raw_args[idx], &raw, &rawlen) : (raw = "", rawlen = 0, "\"\""));
                    Token *st = arena_alloc(sizeof(Token));
                    st->kind = TK_STR;
                    st->kw = ID_NONE;
                    st->str = str_intern(raw, rawlen);
                    st->len = rawlen;
                    st->ptr = spelling;
                    st->val = (int)strlen(spelling);
                    st->filename = b->filename;
                    st->lineno = b->lineno;
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
                splice_va(&rhead, &rtail, m, (vs < 32 && (m->hh_mask & (1u << vs))) ? raw_args : args, argc, b);
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
                splice_va(&rhead, &rtail, m, (idx < 32 && (m->hh_mask & (1u << idx))) ? raw_args : args, argc, b);
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
                // The spliced-in argument tokens may already carry a
                // no_space_after flag set by a *different*, unrelated
                // adjacency check from an earlier expansion layer (e.g.
                // SYM_FUNC_END(name) substitutes name -> "write_ibpb" and
                // correctly marks it "no space before the following comma"
                // in SYM_END(name, SYM_T_FUNC); that same "write_ibpb"
                // token is then reused, via copy_token(), as SYM_END's own
                // *own* `name` argument in ".type name sym_type" - where a
                // real space follows. Left alone, the stale flag from the
                // first expansion survives into the second and makes
                // pp_tokens_to_text() glue "write_ibpb" straight onto
                // "sym_type"/"STT_FUNC" with no separator at all.
                // Recompute it fresh from *this* body position every time,
                // instead of only ever setting it true and never clearing
                // a leftover true from the argument's own prior life.
                Token *before = rtail;
                splice_tokens(&rhead, &rtail, (idx < 32 && (m->hh_mask & (1u << idx))) ? raw_args[idx] : args[idx]);
                if (rtail && rtail != before)
                    rtail->no_space_after = b->next && b->next != end &&
                        b->next->kind != TK_EOF && !str_needs_space(b, b->next);
                continue;
            }
            append_one(&rhead, &rtail, b);
            continue;
        }
        Token *c = copy_token(b);
        c->next = NULL;
        // A plain (non-parameter, non-#/## ) body token directly adjacent
        // to the *next* body token (e.g. "/" before "system" in
        // "TRACE_INCLUDE_PATH/system.h") needs this recorded explicitly:
        // if that next token is itself a substituted parameter, its
        // replacement's spelling lives in a different buffer, so the
        // ordinary pointer-adjacency check in str_needs_space() can never
        // see `c` as touching it.
        c->no_space_after = b->next && b->next != end &&
            b->next->kind != TK_EOF && !str_needs_space(b, b->next);
        if (rtail) rtail->next = c;
        else
            rhead = c;
        rtail = c;
    }
    return rhead;
}
// Actual-argument capacity for a function-like macro call. Grows
// dynamically (doubling) instead of using a small fixed cap: variadic
// wrapper macros like the kernel's "#define PARAMS(args...) args" are
// invoked with however many comma-separated tokens the caller's own
// expanded arguments happen to contain (e.g. TRACE_EVENT's PARAMS(proto)
// after proto's own TP_PROTO(...) has argument-prescan-expanded into a
// bare list, or a __print_symbolic() table of "{ CODE, "name" }" entries
// whose embedded commas each split into their own argument - the kernel's
// show_nfs4_status() alone has ~150 such entries, ~300 arguments), easily
// exceeding a fixed cap even though the macro itself takes "any number of
// args".
#define INITIAL_CALL_ARGS 64
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
    if (name == kw_date) {
        time_t now = time(NULL);
        struct tm *tm = localtime(&now);
        char buf[16];
        static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        snprintf(buf, sizeof(buf), "%s %2d %d",
                 months[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
        out_append(syn_str(buf, strlen(buf), t));
        return;
    }
    if (name == kw_time) {
        time_t now = time(NULL);
        struct tm *tm = localtime(&now);
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                 tm->tm_hour, tm->tm_min, tm->tm_sec);
        out_append(syn_str(buf, strlen(buf), t));
        return;
    }
    if (name == kw_base_file) {
        // Always the top-level main input file, unlike __FILE__ which
        // tracks whatever file is currently being read (see pp_base_file).
        char *fn = pp_base_file ? pp_base_file : (t->filename ? t->filename : "");
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
    if (name == kw_has_include || name == kw_has_include_next || name == kw_has_c_attribute || name == kw_has_builtin) {
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
    // Hide set (C99 6.10.3.4p2): a token is not re-expanded as any macro
    // name already painted on it — its own name included, which stops
    // direct and mutual recursion after one cycle (glibc's `alloca` <->
    // rcc's `__builtin_alloca` alias ping-pong included). A global
    // "currently expanding" flag must NOT also block: metalang99's eval
    // machine legitimately invokes a continuation macro (e.g.
    // ML99_PRIV_EVAL_0callUneval_K) while an EARLIER, still-active frame
    // of the same name sits deeper in the nested rescan (its DEFER
    // trampoline keeps outer frames alive via frame_floor); the old
    // `m->disabled` flag silently froze the whole machine, leaving
    // ML99_EVAL continuations unexpanded (datatype99/metalang99 tests.c
    // stalled with REC_NEXT residue). Pathological runaway recursion that
    // somehow escapes the hide set stays bounded by the nframes cap below.
    if (!m || hs_contains(t->blue, m)) {
        out_append(t);
        return;
    }
    if (!m->is_function) {
        if (nframes > 4096) {
            out_append(t);
            return;
        }
        // An object-like macro's replacement list can still use ##
        // (C11 6.10.3.3p1 applies to both object-like and function-like
        // macros) -- e.g. `#define FOO a ## b`, or config.h's own
        // `#define X val ## SUFFIX`. The plain push_expansion() path
        // below just re-scans the raw body tokens, so a literal `##`
        // token passed through unresolved, AND (with no ##-adjacency
        // awareness at all outside subst_range()) its neighboring
        // identifiers got ordinary macro expansion first -- backwards
        // from the standard's "## operands are not pre-expanded" rule.
        // Route through subst_range() (with no parameters/arguments;
        // safe since every args/raw_args access there is gated on
        // m->is_variadic or an in-range parameter index, neither of
        // which an object-like macro ever has) whenever the body
        // actually contains a `##`, to paste it the same way a
        // function-like macro's body would. Skipped otherwise to keep
        // the overwhelmingly common plain-value object-like macro on
        // its original, cheaper path.
        bool has_hashhash = false;
        for (Token *b = m->body; b && b->kind != TK_EOF; b = b->next) {
            if (b->kind == TK_PUNCT && b->len == 2 && b->ptr[0] == '#' && b->ptr[1] == '#') {
                has_hashhash = true;
                break;
            }
        }
        Token *body = has_hashhash ? subst_range(m, m->body, NULL, NULL, NULL, 0) : m->body;
        push_expansion(body, m, t);
        // The macro name `t` may have been immediately followed (no source
        // whitespace) by the next token in the enclosing scan, e.g.
        // "TRACE_INCLUDE_PATH/system.h" in the kernel's define_trace.h.
        // Stringizing the expansion result must see that same tightness
        // between the *replacement* and that following token, not fall
        // back to "needs space" just because the replacement's tokens live
        // in the #define line's own buffer (see frame_pull()).
        if (t->no_space_after || (t->next && t->next->kind != TK_EOF && !str_needs_space(t, t->next)))
            frames->tight_after = true;
        return;
    }
    Token *lp = xp_next();
    if (!ptok(lp, "(")) {
        xp_unget(lp);
        out_append(t);
        return;
    }
    if (nframes > 4096) {
        xp_unget(lp);
        out_append(t);
        return;
    }
    int args_cap = INITIAL_CALL_ARGS;
    Token **args = calloc(args_cap, sizeof(Token *)), **tails = calloc(args_cap, sizeof(Token *));
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
        // xp_next() returns bare NULL (not &mark_eof) when called with
        // xp_no_raw set (i.e. from inside a nested expand_list() — see its
        // frame_floor handling) and the current frame's tokens run out
        // before a balancing ')' is found: a macro invocation whose
        // arguments are themselves produced by another macro's expansion,
        // and that expansion's token list ends mid-argument-list. Treat it
        // exactly like &mark_eof (abandon the call, replay collected
        // tokens verbatim) instead of falling through to dereference NULL
        // as if it were an ordinary token.
        if (x == &mark_eof || !x) {
            if (!x) x = &mark_eof;
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
            free(args);
            free(tails);
            return;
        }
        if (ptok(x, "(")) depth++;
        else if (ptok(x, ")")) {
            if (--depth == 0) {
                rp = x;
                break;
            }
        } else if (ptok(x, ",") && depth == 1) {
            argc++;
            if (argc >= args_cap) {
                int new_cap = args_cap * 2;
                args = realloc(args, sizeof(Token *) * new_cap);
                tails = realloc(tails, sizeof(Token *) * new_cap);
                memset(args + args_cap, 0, sizeof(Token *) * (new_cap - args_cap));
                memset(tails + args_cap, 0, sizeof(Token *) * (new_cap - args_cap));
                args_cap = new_cap;
            }
            any = false;
            continue;
        }
        x->next = NULL;
        if (tails[argc]) tails[argc]->next = x;
        else
            args[argc] = x;
        tails[argc] = x;
        any = true;
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
        free(args);
        free(tails);
        return;
    }
    if (m->param_len > args_cap) {
        int new_cap = m->param_len;
        args = realloc(args, sizeof(Token *) * new_cap);
        memset(args + args_cap, 0, sizeof(Token *) * (new_cap - args_cap));
        args_cap = new_cap;
    }
    while (argc < m->param_len) args[argc++] = NULL;
    int nargs = argc > 0 ? argc : 1;
    Token **exp_args = calloc(nargs, sizeof(Token *)), **args_copy = calloc(nargs, sizeof(Token *));
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
    push_expansion(subst, m, t);
    free(args);
    free(tails);
    free(exp_args);
    free(args_copy);
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
// __has_builtin(NAME): is a `__builtin_*` identifier one rcc's parser/
// codegen actually recognizes and dispatches on by exact name (parser.c's
// declspec()/unary() builtin chain, codegen.c's gen_funcall() bi_s_* table,
// and preprocess.c's __builtin_X -> library-name macro aliases below)?
// Table is a sorted, mechanically extracted list of every literal
// "__builtin_*" string this source tree dispatches on; kept sorted for
// bsearch(). Not exhaustive of every name GCC/clang recognize (rcc doesn't
// implement every GCC builtin), but accurate for what rcc itself supports.
static int builtin_name_cmp(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}
static int64_t has_builtin_val(const char *name) {
    static const char *builtin_names[] = {
        "__builtin___confstr_chk",
        "__builtin___confstr_chk_warn",
        "__builtin___fprintf_chk",
        "__builtin___getcwd_chk",
        "__builtin___getcwd_chk_warn",
        "__builtin___getdomainname_chk",
        "__builtin___getdomainname_chk_warn",
        "__builtin___getgroups_chk",
        "__builtin___getgroups_chk_warn",
        "__builtin___gethostname_chk",
        "__builtin___gethostname_chk_warn",
        "__builtin___getlogin_r_chk",
        "__builtin___getlogin_r_chk_warn",
        "__builtin___getwd_chk",
        "__builtin___getwd_warn",
        "__builtin___memcmp_chk",
        "__builtin___memcpy_chk",
        "__builtin___mempcpy_chk",
        "__builtin___memmove_chk",
        "__builtin___memset_chk",
        "__builtin___pread_chk",
        "__builtin___pread_chk_warn",
        "__builtin___printf_chk",
        "__builtin___read_chk",
        "__builtin___read_chk_warn",
        "__builtin___readlink_chk",
        "__builtin___readlink_chk_warn",
        "__builtin___readlinkat_chk",
        "__builtin___readlinkat_chk_warn",
        "__builtin___snprintf_chk",
        "__builtin___sprintf_chk",
        "__builtin___strcat_chk",
        "__builtin___strcpy_chk",
        "__builtin___stpcpy_chk",
        "__builtin___strlen_chk",
        "__builtin___strncat_chk",
        "__builtin___strncpy_chk",
        "__builtin___ttyname_r_chk",
        "__builtin___ttyname_r_chk_warn",
        "__builtin___vfprintf_chk",
        "__builtin___vsnprintf_chk",
        "__builtin___vsprintf_chk",
        "__builtin_abort",
        "__builtin_abs",
        "__builtin_add_overflow",
        "__builtin_add_overflow_p",
        "__builtin_alloca",
        "__builtin_apply",
        "__builtin_apply_args",
        "__builtin_assume_aligned",
        "__builtin_bswap16",
        "__builtin_bswap32",
        "__builtin_bswap64",
        "__builtin_calloc",
        "__builtin_choose_expr",
        "__builtin_cimag",
        "__builtin_cimagf",
        "__builtin_classify_type",
        "__builtin_clear_padding",
        "__builtin_clrsb",
        "__builtin_clrsbl",
        "__builtin_clrsbll",
        "__builtin_clz",
        "__builtin_clzl",
        "__builtin_clzll",
        "__builtin_complex",
        "__builtin_conj",
        "__builtin_conjf",
        "__builtin_conjl",
        "__builtin_constant_p",
        "__builtin_copysign",
        "__builtin_copysignf",
        "__builtin_copysignl",
        "__builtin_cpu_init",
        "__builtin_cpu_supports",
        "__builtin_creal",
        "__builtin_crealf",
        "__builtin_ctz",
        "__builtin_ctzl",
        "__builtin_ctzll",
        "__builtin_dynamic_object_size",
        "__builtin_exit",
        "__builtin_expect",
        "__builtin_fabs",
        "__builtin_fabsf",
        "__builtin_ffs",
        "__builtin_ffsl",
        "__builtin_ffsll",
        "__builtin_fma",
        "__builtin_fmaf",
        "__builtin_fmal",
        "__builtin_fmax",
        "__builtin_fmaxf",
        "__builtin_fmin",
        "__builtin_fminf",
        "__builtin_fpclassify",
        "__builtin_fpclassifyf",
        "__builtin_fpclassifyl",
        "__builtin_fprintf",
        "__builtin_frame_address",
        "__builtin_free",
        "__builtin_has_attribute",
        "__builtin_huge_val",
        "__builtin_huge_valf",
        "__builtin_huge_vall",
        "__builtin_ia32_lfence",
        "__builtin_ia32_mfence",
        "__builtin_ia32_pause",
        "__builtin_ia32_pshufb128",
        "__builtin_ia32_rsqrtps",
        "__builtin_ia32_sfence",
        "__builtin_ia32_sqrtpd",
        "__builtin_ia32_sqrtps",
        "__builtin_ia32_sqrtsd",
        "__builtin_ia32_sqrtss",
        "__builtin_inf",
        "__builtin_inff",
        "__builtin_infl",
        "__builtin_isfinite",
        "__builtin_isfinitef",
        "__builtin_isfinitel",
        "__builtin_isinf",
        "__builtin_isinff",
        "__builtin_isinfl",
        "__builtin_isnan",
        "__builtin_isnanf",
        "__builtin_isnanl",
        "__builtin_isnormal",
        "__builtin_isnormalf",
        "__builtin_isnormall",
        "__builtin_labs",
        "__builtin_llabs",
        "__builtin_longjmp",
        "__builtin_malloc",
        "__builtin_memcmp",
        "__builtin_memcpy",
        "__builtin_memmove",
        "__builtin_memset",
        "__builtin_mul_overflow",
        "__builtin_mul_overflow_p",
        "__builtin_nan",
        "__builtin_nanf",
        "__builtin_nanl",
        "__builtin_nans",
        "__builtin_nansf",
        "__builtin_nansl",
        "__builtin_object_size",
        "__builtin_offsetof",
        "__builtin_parity",
        "__builtin_parityl",
        "__builtin_parityll",
        "__builtin_popcount",
        "__builtin_popcountl",
        "__builtin_popcountll",
        "__builtin_pow",
        "__builtin_powf",
        "__builtin_prefetch",
        "__builtin_printf",
        "__builtin_puts",
        "__builtin_realloc",
        "__builtin_return",
        "__builtin_return_address",
        "__builtin_setjmp",
        "__builtin_shuffle",
        "__builtin_shufflevector",
        "__builtin_signbit",
        "__builtin_signbitf",
        "__builtin_signbitl",
        "__builtin_snprintf",
        "__builtin_sprintf",
        "__builtin_strcat",
        "__builtin_strchr",
        "__builtin_strcmp",
        "__builtin_strcpy",
        "__builtin_strdup",
        "__builtin_strlen",
        "__builtin_strncat",
        "__builtin_strncmp",
        "__builtin_strncpy",
        "__builtin_strrchr",
        "__builtin_sub_overflow",
        "__builtin_sub_overflow_p",
        "__builtin_thread_pointer",
        "__builtin_trap",
        "__builtin_types_compatible_p",
        "__builtin_unreachable",
        "__builtin_va_arg",
        "__builtin_va_arg_pack",
        "__builtin_va_arg_pack_len",
        "__builtin_va_copy",
        "__builtin_va_end",
        "__builtin_va_start",
        "__builtin_vprintf",
        "__builtin_vsnprintf",
        "__builtin_vsprintf",
    };
    if (bsearch(&name, builtin_names, sizeof(builtin_names) / sizeof(builtin_names[0]),
                sizeof(builtin_names[0]), builtin_name_cmp))
        return 1;
    // Unlike real GCC/clang (where __builtin_alloca et al. are genuine
    // front-end-recognized identifiers, never macros, so __has_builtin's
    // argument reaches it unexpanded), rcc implements several __builtin_X
    // names as plain preprocessor object macros that alias straight to
    // the underlying library function name (see the __builtin_X ->
    // library-name aliases below, e.g.
    // define_pre("__builtin_alloca", "alloca")) -- an internal
    // codegen-dispatch convenience. __has_builtin's argument DOES get
    // ordinary macro expansion here (matching GCC's real behavior for
    // genuine user macros, e.g. `#define FOO __builtin_expect` then
    // __has_builtin(FOO)), so by the time this function runs, one of
    // rcc's own aliases has already silently turned "__builtin_alloca"
    // into the bare "alloca". Re-synthesize the __builtin_-prefixed
    // spelling and check that too, so both forms answer identically.
    size_t len = strlen(name);
    if (len < 64) {
        char buf[64 + 10];
        snprintf(buf, sizeof(buf), "__builtin_%s", name);
        const char *key = buf;
        if (bsearch(&key, builtin_names, sizeof(builtin_names) / sizeof(builtin_names[0]),
                    sizeof(builtin_names[0]), builtin_name_cmp))
            return 1;
    }
    return 0;
}
static int64_t eval_pp_expr_tok(Token **pp);
// #if-expression primary: defined()/__has_include()/__has_c_attribute(),
// numeric/string literals, parenthesized subexpr, unary !/-/~/+.
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
            return spec ? resolve_include(lvl->filename, lvl->filename, spec, is_angle, NULL, false) != NULL : 0;
        }
        if (nm == kw_has_c_attribute) {
            t = t->next;
            if (ptok(t, "(")) t = t->next;
            char *text = gather_spellings(t, ")", &t);
            if (ptok(t, ")")) t = t->next;
            *pp = t;
            return has_c_attribute_val(text);
        }
        if (nm == kw_has_builtin) {
            t = t->next;
            if (ptok(t, "(")) t = t->next;
            char *text = gather_spellings(t, ")", &t);
            if (ptok(t, ")")) t = t->next;
            *pp = t;
            return has_builtin_val(text);
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
        // A hex/octal (or over-large decimal) constant whose value does not
        // fit in intmax_t is unsigned even without a U suffix (C23 6.4.4.1).
        // Without this, 0xFFFFFFFFFFFFFFFF was the signed value -1, so e.g.
        // `#if SIZE_MAX % UINT_MAX` (0 mathematically) evaluated as signed
        // -1 % 0xFFFFFFFF and came out non-zero (lmdb's mdb.c two's-
        // complement sanity #if).
        if ((uint64_t)t->val > (uint64_t)INT64_MAX) pp_expr_unsigned = true;
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
    if (ptok(t, "~")) {
        t = t->next;
        int64_t val = ~eval_primary_tok(&t);
        *pp = t;
        return val;
    }
    if (ptok(t, "+")) {
        t = t->next;
        int64_t val = eval_primary_tok(&t);
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
            if (!rhs)
                val = 0;
            else if (pp_expr_unsigned)
                val = (int64_t)((uint64_t)val / (uint64_t)rhs);
            else
                val = val / rhs;
        } else if (ptok(t, "%")) {
            t = t->next;
            int64_t rhs = eval_primary_tok(&t);
            if (!rhs)
                val = 0;
            else if (pp_expr_unsigned)
                val = (int64_t)((uint64_t)val % (uint64_t)rhs);
            else
                val = val % rhs;
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
        if (t->kind == TK_CNL) {
            // Newline embedded in a still-open /* */ comment: per the
            // standard's phase ordering, comments (including any newlines
            // inside them) are removed before directive lines are
            // delimited, so this must NOT end the directive - only a real
            // TK_NL does. Still advance the line counter for diagnostics.
            advance_line();
            continue;
        }
        if (t->kind == TK_NL) {
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
        char *params_buf[32];
        char **params = params_buf;
        int np = 0;
        int params_cap = 32;
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
                    if (np >= params_cap) {
                        int new_cap = params_cap * 2;
                        char **np2 = arena_alloc(sizeof(char *) * new_cap);
                        memcpy(np2, params, sizeof(char *) * params_cap);
                        params = np2;
                        params_cap = new_cap;
                    }
                    params[np++] = b->name;
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
    if (dn == dn_include || dn == dn_include_next) {
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
        char *disp = NULL;
        char *path = dn == dn_include_next ? resolve_include_next(lvl->fpath, spec, is_angle)
                                           : resolve_include(lvl->fpath, lvl->filename, spec, is_angle, &disp, false);
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
        push_level(disp ? disp : inc_fpath, inc_fpath, contents);
        dep_add(inc_fpath);
        // -E output must mark the header's ENTRY even when the header
        // emits no tokens of its own (a macro-only header like glibc's
        // bits/signum-generic.h is all #defines): real cpp prints a
        // linemarker for every file entered, and configure-style probes
        // (zsh's "where signal.h is located") extract the signal-macro
        // header paths from those markers. pp_print_tokens() only emits
        // a marker when a TOKEN's filename changes, so a tokenless
        // header never appeared and zsh's configure failed with
        // "SIGNAL MACROS NOT FOUND". A zero-length synthetic token here
        // triggers the entry marker without printing anything.
        if (opt_E) {
            Token *mt = arena_alloc(sizeof(Token));
            mt->kind = TK_IDENT;
            mt->kw = ID_NONE;
            mt->filename = inc_fpath;
            mt->lineno = 1;
            mt->ptr = NULL;
            mt->len = 0;
            mt->next = NULL;
            out_append(mt);
        }
        return;
    }
    if (dn == dn_embed) {
        Token *first = name->next;
        char *spec = NULL;
        bool is_angle = false;
        int limit = -1; // -1 = no limit
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
        // Parse optional embed parameters: limit(N)
        Token *parm = name->next;
        // skip past the filename token(s) to find parameters
        while (parm && parm->kind != TK_EOF) {
            if (parm->kind == TK_IDENT && !strcmp(parm->name, "limit") &&
                ptok(parm->next, "(")) {
                Token *num = parm->next->next;
                if (num && num->kind == TK_NUM) {
                    limit = (int)num->val;
                }
            }
            parm = parm->next;
        }
        char *path = resolve_include(lvl->fpath, lvl->filename, spec, is_angle, NULL, true);
        if (!path) {
            fprintf(stderr, "%s:%d: error: #embed file '%s' not found\n", lvl->fpath, lvl->reported_line, spec);
            exit(1);
        }
        char *inc_fpath = full_path(path);
        FILE *fp = fopen(path, "rb");
        if (!fp) {
            fprintf(stderr, "%s:%d: error: cannot open #embed file '%s'\n", lvl->fpath, lvl->reported_line, path);
            exit(1);
        }
        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (fsize < 0) {
            fclose(fp);
            return;
        }
        if (limit >= 0 && fsize > limit) fsize = limit;
        if (fsize == 0) {
            fclose(fp);
            return;
        }
        // Build comma-separated hex byte literals: "0xNN, 0xNN, ..."
        // 6 bytes per element ("0xNN, ") + 1 for NUL, with comma dropped on last
        size_t bufsz = (size_t)fsize * 6 + 1;
        char *buf = arena_alloc(bufsz);
        size_t pos = 0;
        unsigned char *data = arena_alloc((size_t)fsize);
        size_t nread = fread(data, 1, (size_t)fsize, fp);
        fclose(fp);
        for (size_t i = 0; i < nread; i++) {
            if (i > 0) buf[pos++] = ',';
            buf[pos++] = '0';
            buf[pos++] = 'x';
            unsigned char hi = (unsigned char)(data[i] >> 4);
            unsigned char lo = (unsigned char)(data[i] & 0xf);
            buf[pos++] = (char)(hi < 10 ? '0' + hi : 'a' + hi - 10);
            buf[pos++] = (char)(lo < 10 ? '0' + lo : 'a' + lo - 10);
        }
        buf[pos] = '\0';
        push_level(inc_fpath, inc_fpath, buf);
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
        // Real GCC promotes #warning to a hard error under -Werror
        // (`[-Werror=cpp]`) -- verified directly -- but NOT under
        // -pedantic-errors alone, so this must gate on opt_werror_flag
        // (bare -Werror only), never the broader opt_Werror also set by
        // -pedantic-errors (see main.c's own detailed comment on that
        // distinction). Found via test_muon's own `common/28 try
        // compile` capability probe, which specifically checks that
        // `#warning` promotes under -Werror.
        if (opt_werror_flag) {
            fprintf(stderr, "%s:%d: error: %s\n", lvl->filename, lvl->reported_line, msg);
            exit(1);
        }
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
// Rebuild a re-parseable quoted spelling from decoded string-literal bytes.
// concat_strings() merges the *decoded* content of adjacent string tokens
// (e.g. ".ascii ns \"\\0\"" with an empty ns decodes "\0" to one raw NUL
// byte), and the merged token's ->ptr/->val become the only text that ever
// reaches -E output or the assembler re-lex — tok_spelling() prefers ->ptr
// verbatim once it's set (see below), it never falls back to re-encoding
// ->str. Any decoded byte equal to the lexer's two loop-termination
// sentinels (see the `while (*p && *p != '"' && *p != '\n')` string scan)
// MUST come back out escaped, or the string looks unterminated the moment
// this text is re-lexed: a raw NUL reads as "past end of buffer" and a raw
// newline reads as "unterminated line". `"` and `\` still need their usual
// escaping to stay inside the quotes as literal content.
static char *build_quoted_spelling(const char *bytes, int len, int *out_len) {
    char *sp = arena_alloc((size_t)len * 4 + 3);
    int sn = 0;
    sp[sn++] = '"';
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)bytes[i];
        if (c == '"' || c == '\\') {
            sp[sn++] = '\\';
            sp[sn++] = (char)c;
        } else if (c == '\0') {
            sp[sn++] = '\\';
            sp[sn++] = '0';
        } else if (c == '\n') {
            sp[sn++] = '\\';
            sp[sn++] = 'n';
        } else {
            sp[sn++] = (char)c;
        }
    }
    sp[sn++] = '"';
    *out_len = sn;
    return sp;
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
            int sn;
            char *sp = build_quoted_spelling(merged, mlen, &sn);
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
            int sn;
            char *sp = build_quoted_spelling(merged, len1 + len2, &sn);
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

// -E output. Same source-adjacency logic as pp_tokens_to_text() (see its
// comment): the C lexer splits assembly/linker-script dot-prefixed names
// ("*.hash", ".text", "LINUX_2.6") into separate punctuation + identifier/
// number tokens exactly as it would "a.b.c" in real C, so unconditionally
// separating every token with a space — what this used to do — corrupts
// any -E consumer that cares about exact spelling, not just whitespace-
// insensitive C: a linker-script "cpp -P" pass (kbuild's cmd_cpp_lds_S)
// turns ".hash" into ". hash", which GNU ld's script grammar reads as the
// location-counter symbol "." followed by a bare "hash" — a parse error
// (or, worse, an entirely different symbol table). Real cpp/cc1 preserve
// byte-adjacency; so must this.
void pp_print_tokens(Token *tok, FILE *out) {
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
            if (!first_on_line) fputc('\n', out);
            fprintf(out, "# %d \"%s\"\n", ln, fn);
            cur_line = ln;
            cur_file = fn;
            first_on_line = true;
            prev_sp = NULL;
        }
        while (cur_line < ln) {
            fputc('\n', out);
            cur_line++;
            first_on_line = true;
            prev_sp = NULL;
        }
        int sl;
        char *sp = tok_spelling(tok, &sl);
        bool adjacent = (prev_sp && sp && prev_sp + prev_sl == sp) ||
            (prev_tok && prev_tok->no_space_after);
        if (!first_on_line && !adjacent) fputc(' ', out);
        if (sp && sl > 0) fwrite(sp, 1, (size_t)sl, out);
        first_on_line = false;
        prev_sp = sp;
        prev_sl = sl;
        prev_tok = tok;
    }
    fputc('\n', out);
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
            if (m->is_variadic) total += 4; // ',' + '.' + '.' + '.'
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
        n += snprintf(buf + n, total - n + 1, "#define %s", m->name);
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
    for (int i = 0; i < nb_quote_include_paths; i++) printf("include: =%s\n", quote_include_paths[i]);
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
    // #pragma once is scoped per translation unit, not per process: a
    // multi-file single invocation (`rcc a.c b.c -o prog`, no -c) calls
    // preprocess() once per input file in the same process, so this must
    // reset here or a header #pragma-once'd while compiling a.c is
    // silently (and wrongly) skipped when b.c tries to #include it too.
    once_files = NULL;
    static char *builtin_expect_params[] = {"x", "y"};
    if (!macros_inited) {
#define define_pre(name, value) define_macro(name, false, NULL, 0, value)
        define_pre("__has_include", "1");
        define_pre("__has_c_attribute", "1");
        define_pre("__has_include_next", "1");
        define_pre("__has_builtin", "1");
#include "gcc_predefined.h"
        // rcc's own self-identification macro, matching every other
        // compiler's practice of defining both a GCC-compat macro (for
        // header portability -- see gcc_predefined.h's __GNUC__ etc.
        // above) AND a compiler-specific one so user/build-system code
        // can detect rcc specifically (`#ifdef __RCC__`), the same way
        // `__clang__` or tcc's `__TINYC__` sit alongside `__GNUC__`.
        define_pre("__RCC__", "1");
        // __MUSL__: musl-gcc's spec only redirects include/lib paths and
        // predefines no __MUSL__ itself, so a native musl build injects it
        // here. glibc macros (__GLIBC__/__GLIBC_MINOR__/__GLIBC_PREREQ) are
        // deliberately NOT injected: they must come from the target's own
        // <features.h> (glibc) or stay absent (musl/mingw). A host-glibc
        // rcc cross-compiling to musl would otherwise leak __GLIBC__ into
        // every musl TU, and the prelude re-adds it after clear_macros()
        // so a -U__GLIBC__ on the command line can never undo it.
#ifdef __MUSL__
        define_pre("__MUSL__", "1");
#endif
        // GCC atomic builtins used by libgit2/libgc for atomic operations.
        // Map to __atomic_* builtins which rcc supports.
        define_pre("__builtin_atomic_arith_add", "__atomic_add_fetch");
        define_pre("__builtin_atomic_arith_sub", "__atomic_sub_fetch");
        define_pre("__builtin_atomic_arith_or", "__atomic_or_fetch");
        // __SSIZE_TYPE__ needed by POSIX's ssize_t (stddef.h).
        define_pre("__SSIZE_TYPE__", "long int");
        // __COUNTER__ must exist as a macro so `#ifdef __COUNTER__` /
        // `defined(__COUNTER__)` see it (metalang99/datatype99 gate
        // ML99_GEN_SYM on exactly that guard). Expansion of the token in
        // code still goes through kw_counter below (yielding 0, 1, 2, ...
        // per call); this entry only satisfies the preprocessor-query
        // forms, and the #if expression evaluator sees the plain value 1.
        define_pre("__COUNTER__", "1");
        // glibc's /usr/include/limits.h ends with `#include_next
        // <limits.h>` (guarded on __GNUC__ && !_GCC_LIMITS_H_) to pull
        // the compiler's own limits.h. That include_next now resolves to
        // rcc's bundled include/limits.h (RCC_INCDIR follows user -I
        // dirs since 30f47f34, so with -I/usr/include glibc's limits.h
        // is reached FIRST and must chain onward itself); predefining
        // _GCC_LIMITS_H_ to skip the chain left INT_MAX et al. undefined
        // for every -I/usr/include TU.
        // __STDC_VERSION__ is baked into gcc_predefined.h at the C23 value;
        // reflect the -std= request instead. C89/C90 (opt_std_version==NULL)
        // has no __STDC_VERSION__ at all, so the predefined one must be
        // removed -- otherwise a -std=c89 build still sees 202311L and pulls
        // in C23-only header branches (e.g. our own <stddef.h> nullptr_t via
        // `typedef typeof(nullptr) nullptr_t;`), which then fail to parse.
        if (opt_std_version)
            define_pre("__STDC_VERSION__", (char *)opt_std_version);
        else
            add_undef("__STDC_VERSION__");
        if (!find_macro("__STDC_FENV_ACCESS__")) define_pre("__STDC_FENV_ACCESS__", "1");
        if (opt_std_version && strcmp(opt_std_version, "202311L") == 0) {
            if (!find_macro("bool")) define_pre("bool", "_Bool");
            if (!find_macro("__bool_true_false_are_defined")) define_pre("__bool_true_false_are_defined", "1");
        }
        if (opt_O1) define_pre("__OPTIMIZE__", "1");
        if (!find_macro("__USE_FORTIFY_LEVEL")) define_pre("__USE_FORTIFY_LEVEL", "0");
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
        define_pre("__builtin_mempcpy", "mempcpy");
        define_pre("__builtin_memset", "memset");
        define_pre("__builtin_strlen", "strlen");
        define_pre("__builtin_stpcpy", "stpcpy");
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
            define_macro("__builtin___mempcpy_chk", true, p4m, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),(__dest)):__builtin_mempcpy(__dest,__src,__len))");
            define_macro("__builtin___memmove_chk", true, p4m, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),(__dest)):__builtin_memmove(__dest,__src,__len))");
            define_macro("__builtin___memset_chk", true, p4, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),(__dest)):__builtin_memset(__dest,__src,__len))");
            define_macro("__builtin___memcmp_chk", true, p4, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),0):__builtin_memcmp(__dest,__src,__len))");
            char *p3s[] = {"__dest", "__src", "__bos", NULL};
            define_macro("__builtin___strcpy_chk", true, p3s, 3, "((__bos)!=(unsigned long long)-1&&(__bos)<(__builtin_strlen(__src)+1)?(abort(),(__dest)):__builtin_strcpy(__dest,__src))");
            define_macro("__builtin___stpcpy_chk", true, p3s, 3, "((__bos)!=(unsigned long long)-1&&(__bos)<(__builtin_strlen(__src)+1)?(abort(),(__dest)):__builtin_stpcpy(__dest,__src))");
            define_macro("__builtin___strncpy_chk", true, p4, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__len)?(abort(),(__dest)):__builtin_strncpy(__dest,__src,__len))");
            define_macro("__builtin___strcat_chk", true, p3s, 3, "((__bos)!=(unsigned long long)-1&&(__bos)<(__builtin_strlen(__dest)+__builtin_strlen(__src)+1)?(abort(),(__dest)):__builtin_strcat(__dest,__src))");
            define_macro("__builtin___strncat_chk", true, p4, 4, "((__bos)!=(unsigned long long)-1&&(__bos)<(__builtin_strlen(__dest)+(__len)+1)?(abort(),(__dest)):__builtin_strncat(__dest,__src,__len))");
            char *p2[] = {"__s", "__bos", NULL};
            define_macro("__builtin___strlen_chk", true, p2, 2, "((__bos)!=(unsigned long long)-1&&(__bos)<(__builtin_strlen(__s)+1)?(abort(),0):__builtin_strlen(__s))");
            // These forward to glibc's own exported __printf_chk /
            // __fprintf_chk / __vfprintf_chk / __sprintf_chk /
            // __vsprintf_chk / __snprintf_chk / __vsnprintf_chk (real
            // linkable symbols implementing the actual runtime bounds
            // checking — see bits/stdio2-decl.h), simply dropping the
            // "builtin_" infix to reach the real name. Previously these
            // were declared with the wrong (too-short, non-variadic)
            // parameter counts: a non-variadic function-like macro whose
            // call site supplies MORE arguments than its declared param
            // count is left completely unexpanded by this preprocessor
            // (silently — no error), so e.g. glibc's real 5-argument
            // __builtin___sprintf_chk(dest, flag, bos, fmt, ...) call
            // never matched the old 3-param definition and every use
            // (including __USE_FORTIFY_LEVEL embedded in the unexpanded
            // argument list) survived as raw, uncompilable text.
            define_macro_va("__builtin___printf_chk", (char *[]){"__flag", "__fmt"}, 2,
                            "__printf_chk(__flag,__fmt,__VA_ARGS__)");
            define_macro_va("__builtin___fprintf_chk", (char *[]){"__stream", "__flag", "__fmt"}, 3,
                            "__fprintf_chk(__stream,__flag,__fmt,__VA_ARGS__)");
            define_macro("__builtin___vfprintf_chk", true, (char *[]){"__stream", "__flag", "__fmt", "__ap"}, 4,
                         "__vfprintf_chk(__stream,__flag,__fmt,__ap)");
            define_macro_va("__builtin___sprintf_chk", (char *[]){"__dest", "__flag", "__bos", "__fmt"}, 4,
                            "__sprintf_chk(__dest,__flag,__bos,__fmt,__VA_ARGS__)");
            define_macro("__builtin___vsprintf_chk", true, (char *[]){"__dest", "__flag", "__bos", "__fmt", "__ap"}, 5,
                         "__vsprintf_chk(__dest,__flag,__bos,__fmt,__ap)");
            define_macro_va("__builtin___snprintf_chk", (char *[]){"__dest", "__len", "__flag", "__bos", "__fmt"}, 5,
                            "__snprintf_chk(__dest,__len,__flag,__bos,__fmt,__VA_ARGS__)");
            define_macro("__builtin___vsnprintf_chk", true,
                         (char *[]){"__dest", "__len", "__flag", "__bos", "__fmt", "__ap"}, 6,
                         "__vsnprintf_chk(__dest,__len,__flag,__bos,__fmt,__ap)");
            // `__foo_chk`/`__foo_chk_warn` (bare and __builtin_-prefixed)
            // forward to the plain, unchecked function, dropping the
            // trailing bufsize/buflen bounds argument -- same behavior
            // as before this comment. Using a FUNCTION-LIKE macro here
            // (not a bare object-macro alias to the plain name) matters
            // even for call sites that never fire: <bits/unistd-decl.h>
            // et al. also *declare* these names with a real prototype,
            // e.g. `__read_chk(int, void*, size_t, size_t)` (4 params).
            // An object macro substitutes the identifier "read" straight
            // into that declaration text, producing
            // `extern ssize_t read(int, void*, size_t, size_t);` --
            // 4 params, conflicting with <unistd.h>'s real 3-param
            // `read` prototype. A function-like macro instead
            // substitutes per matched *parameter*, so dropping the
            // trailing param from the replacement body drops the same
            // slot from the declaration's parameter list too, keeping
            // both forms' arity in sync automatically.
            define_macro("__builtin___read_chk", true, (char *[]){"fd", "buf", "n", "bufsize"}, 4, "read(fd,buf,n)");
            define_macro("__builtin___pread_chk", true, (char *[]){"fd", "buf", "n", "off", "bufsize"}, 5, "pread(fd,buf,n,off)");
            define_macro("__builtin___readlink_chk", true, (char *[]){"path", "buf", "len", "buflen"}, 4, "readlink(path,buf,len)");
            define_macro("__builtin___readlinkat_chk", true, (char *[]){"fd", "path", "buf", "len", "buflen"}, 5, "readlinkat(fd,path,buf,len)");
            define_macro("__read_chk", true, (char *[]){"fd", "buf", "n", "bufsize"}, 4, "read(fd,buf,n)");
            define_macro("__pread_chk", true, (char *[]){"fd", "buf", "n", "off", "bufsize"}, 5, "pread(fd,buf,n,off)");
            define_macro("__readlink_chk", true, (char *[]){"path", "buf", "len", "buflen"}, 4, "readlink(path,buf,len)");
            define_macro("__readlinkat_chk", true, (char *[]){"fd", "path", "buf", "len", "buflen"}, 5, "readlinkat(fd,path,buf,len)");
            define_macro("__getcwd_chk", true, (char *[]){"buf", "size", "buflen"}, 3, "getcwd(buf,size)");
            define_macro("__getwd_chk", true, (char *[]){"buf", "buflen"}, 2, "getwd(buf)");
            define_macro("__confstr_chk", true, (char *[]){"name", "buf", "len", "buflen"}, 4, "confstr(name,buf,len)");
            define_macro("__getgroups_chk", true, (char *[]){"size", "list", "listlen"}, 3, "getgroups(size,list)");
            define_macro("__ttyname_r_chk", true, (char *[]){"fd", "buf", "buflen", "nreal"}, 4, "ttyname_r(fd,buf,buflen)");
            define_macro("__getlogin_r_chk", true, (char *[]){"buf", "buflen", "nreal"}, 3, "getlogin_r(buf,buflen)");
            define_macro("__gethostname_chk", true, (char *[]){"buf", "buflen", "nreal"}, 3, "gethostname(buf,buflen)");
            define_macro("__getdomainname_chk", true, (char *[]){"buf", "buflen", "nreal"}, 3, "getdomainname(buf,buflen)");
            define_macro("__builtin___getcwd_chk", true, (char *[]){"buf", "size", "buflen"}, 3, "getcwd(buf,size)");
            define_macro("__builtin___getwd_chk", true, (char *[]){"buf", "buflen"}, 2, "getwd(buf)");
            define_macro("__builtin___confstr_chk", true, (char *[]){"name", "buf", "len", "buflen"}, 4, "confstr(name,buf,len)");
            define_macro("__builtin___getgroups_chk", true, (char *[]){"size", "list", "listlen"}, 3, "getgroups(size,list)");
            define_macro("__builtin___ttyname_r_chk", true, (char *[]){"fd", "buf", "buflen", "nreal"}, 4, "ttyname_r(fd,buf,buflen)");
            define_macro("__builtin___getlogin_r_chk", true, (char *[]){"buf", "buflen", "nreal"}, 3, "getlogin_r(buf,buflen)");
            define_macro("__builtin___gethostname_chk", true, (char *[]){"buf", "buflen", "nreal"}, 3, "gethostname(buf,buflen)");
            define_macro("__builtin___getdomainname_chk", true, (char *[]){"buf", "buflen", "nreal"}, 3, "getdomainname(buf,buflen)");
            define_macro("__read_chk_warn", true, (char *[]){"fd", "buf", "n", "bufsize"}, 4, "read(fd,buf,n)");
            define_macro("__pread_chk_warn", true, (char *[]){"fd", "buf", "n", "off", "bufsize"}, 5, "pread(fd,buf,n,off)");
            define_macro("__readlink_chk_warn", true, (char *[]){"path", "buf", "len", "buflen"}, 4, "readlink(path,buf,len)");
            define_macro("__readlinkat_chk_warn", true, (char *[]){"fd", "path", "buf", "len", "buflen"}, 5, "readlinkat(fd,path,buf,len)");
            define_macro("__getcwd_chk_warn", true, (char *[]){"buf", "size", "buflen"}, 3, "getcwd(buf,size)");
            define_macro("__getwd_warn", true, (char *[]){"buf", "buflen"}, 2, "getwd(buf)");
            define_macro("__confstr_chk_warn", true, (char *[]){"name", "buf", "len", "buflen"}, 4, "confstr(name,buf,len)");
            define_macro("__getgroups_chk_warn", true, (char *[]){"size", "list", "listlen"}, 3, "getgroups(size,list)");
            define_macro("__ttyname_r_chk_warn", true, (char *[]){"fd", "buf", "buflen", "nreal"}, 4, "ttyname_r(fd,buf,buflen)");
            define_macro("__getlogin_r_chk_warn", true, (char *[]){"buf", "buflen", "nreal"}, 3, "getlogin_r(buf,buflen)");
            define_macro("__gethostname_chk_warn", true, (char *[]){"buf", "buflen", "nreal"}, 3, "gethostname(buf,buflen)");
            define_macro("__getdomainname_chk_warn", true, (char *[]){"buf", "buflen", "nreal"}, 3, "getdomainname(buf,buflen)");
            define_macro("__builtin___read_chk_warn", true, (char *[]){"fd", "buf", "n", "bufsize"}, 4, "read(fd,buf,n)");
            define_macro("__builtin___pread_chk_warn", true, (char *[]){"fd", "buf", "n", "off", "bufsize"}, 5, "pread(fd,buf,n,off)");
            define_macro("__builtin___readlink_chk_warn", true, (char *[]){"path", "buf", "len", "buflen"}, 4, "readlink(path,buf,len)");
            define_macro("__builtin___readlinkat_chk_warn", true, (char *[]){"fd", "path", "buf", "len", "buflen"}, 5, "readlinkat(fd,path,buf,len)");
            define_macro("__builtin___getcwd_chk_warn", true, (char *[]){"buf", "size", "buflen"}, 3, "getcwd(buf,size)");
            define_macro("__builtin___getwd_warn", true, (char *[]){"buf", "buflen"}, 2, "getwd(buf)");
            define_macro("__builtin___confstr_chk_warn", true, (char *[]){"name", "buf", "len", "buflen"}, 4, "confstr(name,buf,len)");
            define_macro("__builtin___getgroups_chk_warn", true, (char *[]){"size", "list", "listlen"}, 3, "getgroups(size,list)");
            define_macro("__builtin___ttyname_r_chk_warn", true, (char *[]){"fd", "buf", "buflen", "nreal"}, 4, "ttyname_r(fd,buf,buflen)");
            define_macro("__builtin___getlogin_r_chk_warn", true, (char *[]){"buf", "buflen", "nreal"}, 3, "getlogin_r(buf,buflen)");
            define_macro("__builtin___gethostname_chk_warn", true, (char *[]){"buf", "buflen", "nreal"}, 3, "gethostname(buf,buflen)");
            define_macro("__builtin___getdomainname_chk_warn", true, (char *[]){"buf", "buflen", "nreal"}, 3, "getdomainname(buf,buflen)");
        }
        define_pre("signbit", "__builtin_signbit");
        define_pre("__builtin_trap", "abort");
        define_macro("__builtin_clear_padding", true, (char *[]){"ptr"}, 1, "__builtin_memset(ptr, 0, sizeof(*(ptr)))");
        // x86 spin-wait / fence intrinsics: real GCC/clang implement
        // these as genuine compiler builtins (no header, no linkable
        // symbol). glibc/kernel spinlock code calls them directly, e.g.
        // curl's lib/curlx bundles a copy of a header that does so
        // without going through <emmintrin.h>'s _mm_pause(), which left
        // an unresolved `__builtin_ia32_pause` at link time.
#ifndef ARCH_ARM64
        define_macro("__builtin_ia32_pause", true, NULL, 0, "__asm__ __volatile__(\"pause\")");
        define_macro("__builtin_ia32_mfence", true, NULL, 0, "__asm__ __volatile__(\"mfence\":::\"memory\")");
        define_macro("__builtin_ia32_lfence", true, NULL, 0, "__asm__ __volatile__(\"lfence\":::\"memory\")");
        define_macro("__builtin_ia32_sfence", true, NULL, 0, "__asm__ __volatile__(\"sfence\":::\"memory\")");
        // __builtin_cpu_supports("feature")/__builtin_cpu_init(): real
        // GCC/clang compiler builtins used for runtime CPU-feature dispatch
        // (e.g. blosc2's blosc_get_cpu_features(), picking an
        // SSE2/AVX2/AVX512 code path; libucl's bundled mum.h: `if
        // (!avx2_support) { __builtin_cpu_init(); avx2_support =
        // __builtin_cpu_supports("avx2") ? 1 : -1; }`). Real GCC compiles
        // __builtin_cpu_supports to a call into libgcc's
        // __cpu_indicator_init()-populated __cpu_model bitmask; rcc instead
        // expands both to statement-expression macros that query CPUID
        // directly at each call site (correct, just not cached/hoisted;
        // __builtin_cpu_init is a true no-op).
        //
        // Deliberately a macro, not a real function injected into every
        // parse's synthetic prelude (as an earlier version of this fix
        // did): a "static inline, never referenced" function body is only
        // stripped by opt.c's eliminate_unused_static_inline() DCE pass,
        // and that pass's own header comment documents it as disabled
        // outright on the mingw/Windows target (a separate, unrelated
        // object-layout/TLS-corruption risk under investigation there).
        // Unconditionally injecting a real function would have left two
        // dead functions bloating every mingw translation unit's .text
        // and, worse, shifted unrelated code layout enough to break
        // several raw-inline-asm byte-pattern regression tests
        // (test_x86_priv_insns et al.) as a pure side effect of the extra,
        // never-DCE'd function bodies. A macro sidesteps this
        // categorically: it expands to zero bytes unless a real call site
        // actually invokes it, on every target uniformly, with no DCE
        // dependency at all.
        //
        // Manual per-character comparison instead of strcmp() avoids any
        // dependency on <string.h> having been included yet. Covers the
        // feature names actually probed by this project's third-party
        // test suite (sse2, avx, avx2, avx512f, avx512bw); add more
        // leaf-7/leaf-1 bits here if a future project needs a name not
        // yet covered.
        define_macro("__builtin_cpu_supports", true, (char *[]){"f"}, 1,
                     "({"
                     "  unsigned __rcc_a,__rcc_b,__rcc_c,__rcc_d,__rcc_r=0;"
                     "  __asm__(\"cpuid\":\"=a\"(__rcc_a),\"=b\"(__rcc_b),\"=c\"(__rcc_c),\"=d\"(__rcc_d):\"a\"(1),\"c\"(0));"
                     "  if ((f)[0]=='s'&&(f)[1]=='s'&&(f)[2]=='e'&&(f)[3]=='2'&&(f)[4]==0) __rcc_r=(__rcc_d>>26)&1;"
                     "  else if ((f)[0]=='a'&&(f)[1]=='v'&&(f)[2]=='x'&&(f)[3]==0) __rcc_r=(__rcc_c>>28)&1;"
                     "  else {"
                     "    unsigned __rcc_a2,__rcc_b2,__rcc_c2,__rcc_d2;"
                     "    __asm__(\"cpuid\":\"=a\"(__rcc_a2),\"=b\"(__rcc_b2),\"=c\"(__rcc_c2),\"=d\"(__rcc_d2):\"a\"(7),\"c\"(0));"
                     "    if ((f)[0]=='a'&&(f)[1]=='v'&&(f)[2]=='x'&&(f)[3]=='2'&&(f)[4]==0) __rcc_r=(__rcc_b2>>5)&1;"
                     "    else if ((f)[0]=='a'&&(f)[1]=='v'&&(f)[2]=='x'&&(f)[3]=='5'&&(f)[4]=='1'&&(f)[5]=='2'&&(f)[6]=='f'&&(f)[7]==0) __rcc_r=(__rcc_b2>>16)&1;"
                     "    else if ((f)[0]=='a'&&(f)[1]=='v'&&(f)[2]=='x'&&(f)[3]=='5'&&(f)[4]=='1'&&(f)[5]=='2'&&(f)[6]=='b'&&(f)[7]=='w'&&(f)[8]==0) __rcc_r=(__rcc_b2>>30)&1;"
                     "  }"
                     "  __rcc_r;"
                     "})");
        define_macro("__builtin_cpu_init", true, NULL, 0, "((void)0)");
#endif
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
        kw_date = str_intern("__DATE__", 8);
        kw_time = str_intern("__TIME__", 8);
        kw_base_file = str_intern("__BASE_FILE__", 13);
        kw_counter = str_intern("__COUNTER__", 11);
        kw_function = str_intern("__FUNCTION__", 12);
        kw_func = str_intern("__func__", 8);
        kw_pretty_function = str_intern("__PRETTY_FUNCTION__", 19);
        kw_has_include = str_intern("__has_include", 13);
        kw_has_include_next = str_intern("__has_include_next", 18);
        kw_has_c_attribute = str_intern("__has_c_attribute", 17);
        kw_has_builtin = str_intern("__has_builtin", 13);
        kw_va_args = str_intern("__VA_ARGS__", 11);
        kw_va_opt = str_intern("__VA_OPT__", 10);
        kw_defined = str_intern("defined", 7);
        kw_true = str_intern("true", 4);
        kw_false = str_intern("false", 5);
        dn_define = str_intern("define", 6);
        dn_undef = str_intern("undef", 5);
        dn_include_next = str_intern("include_next", 12);
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
        dn_embed = str_intern("embed", 5);
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
    pp_base_file = resolved_name;
    lvl = NULL;
    // push_level() splices line continuations itself; passing raw input avoids a
    // double splice that would discard the physical-line counts (breaks __LINE__).
    // Push main source first, then pre-include files on top (LIFO).
    // Pre-includes must process first so their macros are visible.
    push_level(resolved_name, full_path(resolved_name), p);
    for (int i = nb_preinclude - 1; i >= 0; i--) {
        char *inc_path = full_path((char *)preinclude_list[i]);
        // codeql[cpp/path-injection]: -include <file> is a compiler input
        // path, same trust model as the source file argument itself.
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

// Emit one Make dependency rule ("target: prereqs", plus -MP phony rules)
// to an already-open stream. Shared by write_dep_file() (side .d file, for
// -MD/-MMD/-Wp,-MMD,/-MF) and print_dep_rule() (-M/-MM's own stdout/-o
// output, no side file involved).
static void emit_dep_rule(FILE *f, const char *target, const char *main_fpath) {
    // Input read from stdin ("-") isn't a real path on disk — GCC omits it
    // from the prerequisite list rather than emitting an unopenable "-"
    // entry (which trips up kbuild's fixdep).
    fprintf(f, "%s:", target);
    if (strcmp(main_fpath, "-") != 0)
        fprintf(f, " %s", main_fpath);
    for (DepEntry *d = dep_files; d; d = d->next) {
        if (d->path) fprintf(f, " %s", d->path);
    }
    fprintf(f, "\n");
    // -MP: emit an empty phony rule for each prerequisite so a deleted
    // header doesn't break the build with "No rule to make target".
    if (opt_dep_phony) {
        if (strcmp(main_fpath, "-") != 0)
            fprintf(f, "%s:\n", main_fpath);
        for (DepEntry *d = dep_files; d; d = d->next) {
            if (d->path) fprintf(f, "%s:\n", d->path);
        }
    }
}

// Write Make dependency rules to a side ".d" file. Driven by -Wp,-MMD,<file>
// (autotools), the bare -MD/-MMD/-MF/-MT/-MQ/-MP forms (CMake/ninja), or
// -M/-MM combined with an explicit -MF. The rule target defaults to the -o
// object (or a.out); -MT/-MQ override it. The ".d" filename comes from -MF,
// else is derived from the object path by replacing its extension with
// ".d" (matching GCC's -MD-without-MF).
void write_dep_file(const char *out_path, const char *main_fpath) {
    if ((!opt_depfile && !opt_gen_deps) || !main_fpath) return;
    // -M/-MM without -MF print the rule to stdout/-o instead, via
    // print_dep_rule() from main.c — no side .d file in that case.
    if (opt_deps_only && !opt_depfile) return;

    // Resolve the output ".d" path.
    char derived[4096];
    const char *depfile = opt_depfile;
    if (!depfile) {
        const char *base = out_path ? out_path : "a.out";
        const char *dot = strrchr(base, '.');
        // Only treat a '.' in the basename (after the last '/') as an
        // extension, so "dir.x/foo" (no ext) appends ".d" rather than
        // clobbering the directory component.
        const char *slash = strrchr(base, '/');
        if (dot && (!slash || dot > slash))
            snprintf(derived, sizeof(derived), "%.*s.d", (int)(dot - base), base);
        else
            snprintf(derived, sizeof(derived), "%s.d", base);
        depfile = derived;
    }

    // codeql[cpp/path-injection,cpp/world-writable-file-creation]:
    // depfile is a compiler-output path (-MF / -Wp,-MMD, / derived from
    // -o), same trust model as -o.
    FILE *f = fopen(depfile, "w");
    if (!f) {
        fprintf(stderr, "rcc: error: cannot open dependency file '%s'\n", depfile);
        return;
    }
    const char *target = opt_dep_target ? opt_dep_target
                                        : (out_path ? out_path : "a.out");
    emit_dep_rule(f, target, main_fpath);
    fclose(f);
}

// -M/-MM without -MF: print the dependency rule to `f` (stdout, or -o's
// file — wherever -E's own output would go) instead of a side .d file.
void print_dep_rule(FILE *f, const char *out_path, const char *main_fpath) {
    const char *target = opt_dep_target ? opt_dep_target
                                        : (out_path ? out_path : "a.out");
    emit_dep_rule(f, target, main_fpath);
}
