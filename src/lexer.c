// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from chibicc by Rui Ueyama.
#include "rcc.h"
#include <stdarg.h>
#include <ctype.h>
#ifdef HAVE_ICONV
#include <iconv.h>
#endif

// -fexec-charset support: translate one source character (Unicode codepoint)
// to the execution character set. Only unprefixed character constants and
// narrow string literal characters go through this; u8/u/U/L literals and
// numeric escapes are unaffected. Returns the input unchanged when no
// -fexec-charset is given or the conversion is unavailable.
static uint32_t to_exec_charset(uint32_t c) {
#ifdef HAVE_ICONV
    if (!opt_exec_charset)
        return c;
    static iconv_t cd = (iconv_t)-1;
    static const char *cd_charset;
    if (cd_charset != opt_exec_charset) {
        if (cd != (iconv_t)-1)
            iconv_close(cd);
        cd = iconv_open(opt_exec_charset, "UTF-8");
        cd_charset = opt_exec_charset;
    }
    if (cd == (iconv_t)-1)
        return c;
    char in[4];
    size_t inlen;
    if (c < 0x80) {
        in[0] = (char)c;
        inlen = 1;
    } else if (c < 0x800) {
        in[0] = (char)(0xC0 | (c >> 6));
        in[1] = (char)(0x80 | (c & 0x3F));
        inlen = 2;
    } else {
        in[0] = (char)(0xE0 | (c >> 12));
        in[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        in[2] = (char)(0x80 | (c & 0x3F));
        inlen = 3;
    }
    char out[8];
    char *inp = in, *outp = out;
    size_t outlen = sizeof(out);
    if (iconv(cd, &inp, &inlen, &outp, &outlen) == (size_t)-1)
        return c;
    if (outp - out == 1)
        return (uint8_t)out[0];
    return c;
#else
    return c;
#endif
}

// Input string
char *current_input;
char *current_filename;
int current_line_offset = 0;
int line_num = 1;

// Preprocessor lexing mode: the token-based preprocessor drives lex_one()
// directly. In this mode newlines surface as TK_NL tokens (one per logical
// line, comments included as TK_CNL), '#' is an ordinary punctuator, and the
// caller (the preprocessor) owns all line-number accounting.
bool lex_pp_mode = false;
static int pp_pending_cnl; // comment newlines owed to the PP as TK_CNL

// Registry of all lexed source buffers (main file + every include), so error
// reporting can bound its line scan to the buffer that actually owns `loc`.
// Needed once tokens point into per-file buffers instead of one big
// preprocessed text.
typedef struct SrcBuf SrcBuf;
struct SrcBuf {
    SrcBuf *next;
    char *start;
    char *end;
};
static SrcBuf *src_bufs;

void register_source_buffer(char *start, char *end) {
    SrcBuf *b = arena_alloc(sizeof(SrcBuf));
    b->start = start;
    b->end = end;
    b->next = src_bufs;
    src_bufs = b;
}

static char *source_buffer_base(char *loc) {
    for (SrcBuf *b = src_bufs; b; b = b->next)
        if (b->start <= loc && loc < b->end)
            return b->start;
    return current_input;
}
// Tracks the filename from #line directives for error/warning messages.
// Updated unconditionally (not gated on opt_g) so warnings in included
char *current_debug_filename;

// Pick the filename to display in a diagnostic. `file` is the #line-tracked
// filename captured on the token (tok->filename), or NULL. When it names the
// same file as the original source (matching basename) prefer the original
// (shorter, relative) path the user passed on the command line; otherwise use
// the tracked filename so warnings in included headers name the right file.
static const char *display_filename(const char *file) {
    if (!file)
        return current_debug_filename ? current_debug_filename : current_filename;
    const char *dbg_base = file;
    for (const char *p = file; *p; p++)
        if (*p == '/' || *p == '\\')
            dbg_base = p + 1;
    const char *orig_base = current_filename ? current_filename : "";
    for (const char *p = orig_base; *p; p++)
        if (*p == '/' || *p == '\\')
            orig_base = p + 1;
    if (current_filename && strcmp(dbg_base, orig_base) == 0)
        return current_filename;
    return file;
}


// Reports an error and exit.
// cppcheck-suppress va_end_missing
// TODO not-returning attribute
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "\033[1;31merror:\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Compute the reported line number for a location in the preprocessed source.
static int compute_line_no(char *loc) {
    if (!loc)
        return 1;
    char *line = loc;
    while (current_input < line && line[-1] != '\n')
        line--;
    int reported_line = line_num;
    if (current_line_offset == 0 || loc < current_input + current_line_offset) {
        reported_line = 1;
        for (char *p = current_input; p < line; p++)
            if (*p == '\n') reported_line++;
    } else {
        for (char *p = current_input + current_line_offset; p < line; p++)
            if (*p == '\n') reported_line++;
    }
    return reported_line;
}

// Gorgeous error reporting with pointing carets.
// `file` / `lineno` come from the offending token (tok->filename / tok->lineno),
// captured when the token was created so they stay correct after tokenizing has
// advanced the global #line state. Pass file=NULL / lineno=0 to fall back to the
// current lexer position.
// TODO not-returning attribute
static void verror_at(char *loc, int len, const char *file, int lineno,
                      char *fmt, va_list ap) {
    if (!loc) {
        fprintf(stderr, "\033[1;31merror:\033[0m ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        exit(1);
    }
    // Find line containing `loc`, bounded by the buffer that owns it
    // (tokens may point into any included file's buffer, not current_input).
    char *base = source_buffer_base(loc);
    char *line = loc;
    while (base < line && line[-1] != '\n')
        line--;

    char *end = loc;
    while (*end != '\n' && *end != '\0')
        end++;

    int reported_line = lineno > 0 ? lineno : compute_line_no(loc);

    // Print filename and line info
    fprintf(stderr, "\033[1;37m%s:%d: \033[0m", display_filename(file), reported_line);
    fprintf(stderr, "\033[1;31merror:\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    // Print the line
    fprintf(stderr, " %.*s\n", (int)(end - line), line);

    // Print the squiggly line
    int pos = loc - line + 1; // +1 for the space we added above
    for (int i = 0; i < pos; i++) fprintf(stderr, " "); // Indent

    int tilde_len = len > 0 ? len : 1;
    fprintf(stderr, "\033[1;31m^");
    for (int i = 1; i < tilde_len; i++) fprintf(stderr, "~");
    fprintf(stderr, "\033[0m\n");

    exit(1);
}

// Reports an error location and exit.
// cppcheck-suppress va_end_missing
// cppcheck-suppress uninitvar
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, 1, NULL, 0, fmt, ap);
    exit(1);
}

// Like error_tok but prints filename:line: error: message only (no source context).
// Used for inline-asm validation errors so output matches TCC's assembler format.
// cppcheck-suppress va_end_missing
__attribute__((noreturn)) void error_tok_simple(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int lineno = tok && tok->lineno > 0 ? tok->lineno
                                        : (tok && tok->ptr ? compute_line_no(tok->ptr) : 1);
    fprintf(stderr, "%s:%d: error: ",
            display_filename(tok ? tok->filename : NULL), lineno);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

// cppcheck-suppress va_end_missing
// cppcheck-suppress uninitvar
void error_tok(Token *tok, char *fmt, ...) {
    if (!tok) {
        va_list ap;
        va_start(ap, fmt);
        verror_at(NULL, 0, NULL, 0, fmt, ap);
        exit(1);
    }
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->ptr, tok->len, tok->filename, tok->lineno, fmt, ap);
    exit(1);
}


void warn_tok(Token *tok, char *fmt, ...) {
    if (!tok) {
        // warn without location info
        va_list ap;
        va_start(ap, fmt);
        if (opt_Werror)
            fprintf(stderr, "\033[1;31merror:\033[0m ");
        else
            fprintf(stderr, "warning: ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        va_end(ap);
        if (opt_Werror)
            exit(1);
        return;
    }
    // Compute line number
    int reported_line = tok->lineno;
    if (reported_line <= 0) {
        char *line = tok->ptr;
        while (current_input < line && line[-1] != '\n')
            line--;
        reported_line = line_num;
        if (current_line_offset == 0 || tok->ptr < current_input + current_line_offset) {
            reported_line = 1;
            for (char *p = current_input; p < line; p++)
                if (*p == '\n')
                    reported_line++;
        } else {
            for (char *p = current_input + current_line_offset; p < line; p++)
                if (*p == '\n')
                    reported_line++;
        }
    }
    const char *err_file = display_filename(tok->filename);
    // Use basename of file
    const char *base = err_file;
    for (const char *p = err_file; *p; p++)
        if (*p == '/' || *p == '\\')
            base = p + 1;
    va_list ap;
    va_start(ap, fmt);
    if (opt_Werror)
        fprintf(stderr, "%s:%d: \033[1;31merror:\033[0m ", base, reported_line);
    else
        fprintf(stderr, "%s:%d: warning: ", base, reported_line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    if (opt_Werror)
        exit(1);
}

// Create a new token.
static Token *new_token(TokenKind kind, char *start, char *end, int lineno) {
    Token *tok = arena_alloc(sizeof(Token));
    tok->kind = kind;
    tok->kw = ID_NONE;
    tok->ptr = start;
    tok->len = end - start;
    tok->lineno = lineno;
    // Capture the #line-tracked filename on every token (not just with -g)
    // so warn_tok/error_tok report the right file even after tokenizing has
    // advanced current_debug_filename past this token's position.
    tok->filename = current_debug_filename;
    return tok;
}

static bool startswith(char *p, char *q) {
    return strncmp(p, q, strlen(q)) == 0;
}

// Read a punctuator token from p and returns its length.
// Dispatch on the first byte: this runs on every punctuator token, and the
// previous chain cost up to 23 strncmp calls to recognize a bare ';'.
// Reading p[1]/p[2] is safe: the input is NUL-terminated and a NUL simply
// fails the match, so we never scan past the terminator.
static int read_punct(char *p) {
    switch (*p) {
    case '.':
        return (p[1] == '.' && p[2] == '.') ? 3 : 1;
    case '<':
        if (p[1] == '<') return p[2] == '=' ? 3 : 2; // <<= <<
        return p[1] == '=' ? 2 : 1; // <= <
    case '>':
        if (p[1] == '>') return p[2] == '=' ? 3 : 2; // >>= >>
        return p[1] == '=' ? 2 : 1; // >= >
    case '=':
    case '!':
        return p[1] == '=' ? 2 : 1;
    case '+':
        return (p[1] == '+' || p[1] == '=') ? 2 : 1;
    case '-':
        return (p[1] == '-' || p[1] == '=' || p[1] == '>') ? 2 : 1;
    case '&':
        return (p[1] == '&' || p[1] == '=') ? 2 : 1;
    case '|':
        return (p[1] == '|' || p[1] == '=') ? 2 : 1;
    case '*':
    case '/':
    case '%':
    case '^':
        return p[1] == '=' ? 2 : 1;
    case '#':
        return p[1] == '#' ? 2 : 1;
    default:
        return ispunct(*p) ? 1 : 0;
    }
}

static bool is_ident1(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_' || c == '$';
}

static bool is_ident2(char c) {
    return is_ident1(c) || ('0' <= c && c <= '9');
}

static char get_escape_char(char c) {
    switch (c) {
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    case '0': return '\0';
    default: return c;
    }
}

static int from_octal(char c) {
    return c - '0';
}

static int from_hex(char c) {
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static char read_escaped_char(char **new_pos, char *p) {
    if (*p == 'x') {
        p++;
        int val = 0;
        int digit = from_hex(*p);
        if (digit < 0)
            error_at(p, "invalid hex escape");
        while ((digit = from_hex(*p)) >= 0) {
            val = val * 16 + digit;
            p++;
        }
        *new_pos = p;
        return (char)val;
    }

    if ('0' <= *p && *p <= '7') {
        int val = 0;
        int n = 0;
        while (n < 3 && '0' <= *p && *p <= '7') {
            val = val * 8 + from_octal(*p);
            p++;
            n++;
        }
        *new_pos = p;
        return (char)val;
    }

    char c = get_escape_char(*p);
    *new_pos = p + 1;
    return c;
}

/* Lex a single token starting at *pp, advancing *pp and *plineno past it.
 * Whitespace, comments and preprocessor directives are consumed without
 * producing a token, so the scan keeps going until a token is produced or
 * the input runs out.  Returns NULL at end of input.
 * Callers must have set current_input/current_debug_filename (tokenize does).
 *
 * In lex_pp_mode the preprocessor drives the scan: newline then surfaces as
 * a TK_NL token and the caller owns line accounting; a block comment emits
 * one TK_CNL per embedded newline (queued via pp_pending_cnl); '#' lexes as
 * an ordinary punctuator.
 */
Token *lex_one(char **pp, int *plineno) {
    char *p = *pp;
    int cur_lineno = *plineno;
    Token head = {};
    Token *cur = &head;

    if (lex_pp_mode && pp_pending_cnl > 0) {
        pp_pending_cnl--;
        return new_token(TK_CNL, p, p, cur_lineno);
    }

    while (*p) {
        if (cur != &head)
            break; // a token was emitted on the previous round
        // Skip whitespace characters.
        if (isspace(*p)) {
            if (*p == '\n') {
                if (lex_pp_mode) {
                    cur = cur->next = new_token(TK_NL, p, p + 1, cur_lineno);
                    p++;
                    continue;
                }
                cur_lineno++;
            }
            p++;
            continue;
        }

        // Skip preprocessor directives naively
        if (*p == '#' && !lex_pp_mode) {
            // Check for #line directive: # line_number "filename"
            char *q = p + 1;
            while (*q == ' ' || *q == '\t') q++;
            if (isdigit(*q)) {
                // Parse line number
                int n = 0;
                while (isdigit(*q)) {
                    n = n * 10 + (*q - '0');
                    q++;
                }
                while (*q == ' ' || *q == '\t') q++;
                if (*q == '"') {
                    // Parse filename
                    char *fn_start = q + 1;
                    q++;
                    while (*q && *q != '"') q++;
                    if (*q == '"') {
                        current_debug_filename = str_intern(fn_start, q - fn_start);
                        current_line_offset = q + 1 - current_input;
                        line_num = n - 1;
                        cur_lineno = n;
                        p = q + 1;
                        while (*p && *p != '\n') p++;
                        // `# N "file"` numbers the *next* physical line N.
                        // Consume the directive's trailing newline here (rather
                        // than counting it) so cur_lineno stays N for that line;
                        // otherwise the main loop counts it again -> off by one.
                        if (*p == '\n') p++;
                        continue;
                    }
                }
            }
            // Don't skip #pragma pack directives; let the parser handle them
            bool is_pack_pragma = false;
            if (startswith(q, "pragma")) {
                char *r = q + 6;
                while (*r == ' ' || *r == '\t') r++;
                if (startswith(r, "pack"))
                    is_pack_pragma = true;
                else if (startswith(r, "fenv"))
                    is_pack_pragma = true;
                else if (startswith(r, "STDC"))
                    is_pack_pragma = true;
                else if (startswith(r, "unicode")) {
                    // #pragma unicode ScriptName
                    r += 7;
                    while (*r == ' ' || *r == '\t') r++;
                    char *sname = r;
                    while (*r && *r != '\n' && !isspace((unsigned char)*r)) r++;
                    int slen = r - sname;
                    if (slen > 0) {
                        char *name = arena_alloc(slen + 1);
                        memcpy(name, sname, slen);
                        name[slen] = 0;
                        u8ident_allow_script(name);
                    }
                    // Skip rest of line
                    while (*r && *r != '\n') r++;
                    p = r;
                    if (*p == '\n') {
                        p++;
                        cur_lineno++;
                    }
                    continue;
                }
            }
            if (!is_pack_pragma) {
                while (*p && *p != '\n')
                    p++;
                if (*p == '\n') {
                    p++;
                    cur_lineno++;
                }
                continue;
            }
        }

        // Skip line comments
        if (startswith(p, "//")) {
            p += 2;
            while (*p != '\n' && *p != '\0')
                p++;
            continue;
        }

        // Skip block comments
        if (startswith(p, "/*")) {
            char *q = strstr(p + 2, "*/");
            if (!q)
                error_at(p, "unclosed block comment");
            if (lex_pp_mode) {
                // Hand embedded newlines to the PP one at a time as TK_CNL;
                // it owns line accounting and directive-boundary detection.
                for (char *r = p + 2; r < q; r++)
                    if (*r == '\n')
                        pp_pending_cnl++;
                p = q + 2;
                if (pp_pending_cnl > 0) {
                    pp_pending_cnl--;
                    cur = cur->next = new_token(TK_CNL, p, p, cur_lineno);
                }
                continue;
            }
            for (char *r = p + 2; r < q; r++)
                if (*r == '\n') cur_lineno++;
            p = q + 2;
            continue;
        }

        // Numeric literal (integer or floating point)
        if (isdigit(*p) || (*p == '.' && isdigit(p[1]))) {
            char *q = p;
            bool is_float = false;
            // C23 digit separators: a ' continues the number only when
            // followed by a digit of the current base; otherwise it starts
            // a character constant (e.g. acat(0'.')). Before C23 (-std=c11
            // etc.) ' never continues a number: `1'2` is `1` then a
            // character constant, matching GCC's pre-C23 lexing.
            bool dsep = opt_std_version && strcmp(opt_std_version, "202311L") >= 0;

            if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
                p += 2;
                while (isxdigit(*p) || (dsep && *p == '\'' && isxdigit(p[1]))) p++;
                if (*p == '.') {
                    is_float = true;
                    p++;
                    while (isxdigit(*p) || (dsep && *p == '\'' && isxdigit(p[1]))) p++;
                }
                if (*p == 'p' || *p == 'P') {
                    is_float = true;
                    p++;
                    if (*p == '+' || *p == '-') p++;
                    while (isdigit(*p) || (dsep && *p == '\'' && isdigit(p[1]))) p++;
                }
            } else if (*p == '0' && (p[1] == 'o' || p[1] == 'O')) {
                p += 2;
                while ((*p >= '0' && *p <= '7') ||
                       (dsep && *p == '\'' && p[1] >= '0' && p[1] <= '7')) p++;
            } else if (*p == '0' && (p[1] == 'b' || p[1] == 'B')) {
                p += 2;
                while (*p == '0' || *p == '1' ||
                       (dsep && *p == '\'' && (p[1] == '0' || p[1] == '1'))) p++;
            } else {
                while (isdigit(*p) || (dsep && *p == '\'' && isdigit(p[1]))) p++;
                if (*p == '.' && p[1] != '.') {
                    is_float = true;
                    p++;
                    while (isdigit(*p) || (dsep && *p == '\'' && isdigit(p[1]))) p++;
                }
                if (*p == 'e' || *p == 'E') {
                    is_float = true;
                    p++;
                    if (*p == '+' || *p == '-') p++;
                    while (isdigit(*p) || (dsep && *p == '\'' && isdigit(p[1]))) p++;
                }
            }

            // Check for float/imaginary suffix: f/F (incl. _FloatN forms),
            // l/L, i/I/j/J in any order
            int fkind = 0; // 0=double, 1=float, 2=long double
            bool is_imag = false;
            if (is_float) {
                bool have_kind = false;
                for (;;) {
                    if (!have_kind && (*p == 'f' || *p == 'F') && isdigit(p[1])) {
                        // _FloatN suffixes (C23): F16, F32, F64, F128, F32x, F64x
                        p++; // skip F/f
                        if (p[0] == '3' && p[1] == '2') {
                            p += 2;
                            fkind = (*p == 'x' || *p == 'X') ? (p++, 0) : 1; // F32->float, F32x->double
                        } else if (p[0] == '6' && p[1] == '4') {
                            p += 2;
                            fkind = (*p == 'x' || *p == 'X') ? (p++, 2) : 0; // F64->double, F64x->long double
                        } else if (p[0] == '1' && p[1] == '2' && p[2] == '8') {
                            fkind = 2;
                            p += 3; // F128->long double (correct on ARM64; stub on x86)
                        } else if (p[0] == '1' && p[1] == '6') {
                            fkind = 1;
                            p += 2; // F16->float (nearest supported type)
                        }
                        have_kind = true;
                    } else if (!have_kind && (*p == 'f' || *p == 'F')) {
                        fkind = 1;
                        have_kind = true;
                        p++;
                    } else if (!have_kind && (*p == 'l' || *p == 'L')) {
                        fkind = 2;
                        have_kind = true;
                        p++;
                    } else if (!is_imag &&
                               (*p == 'i' || *p == 'I' || *p == 'j' || *p == 'J')) {
                        is_imag = true;
                        p++;
                    } else
                        break;
                }
                // Also handle DD/DF/DL decimal suffixes
                if (!have_kind && !is_imag &&
                    (*p == 'd' || *p == 'D') &&
                    (p[1] == 'd' || p[1] == 'D' || p[1] == 'f' || p[1] == 'F' || p[1] == 'l' || p[1] == 'L')) {
                    if (p[1] == 'd' || p[1] == 'D') fkind = 0;
                    else if (p[1] == 'f' || p[1] == 'F')
                        fkind = 1;
                    else
                        fkind = 2;
                    p += 2;
                }
            } else {
                // Integer suffix: u/U, l/L, C23 wb/WB (_BitInt), and the
                // imaginary i/I/j/J extension
                for (;;) {
                    if (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L')
                        p++;
                    else if ((*p == 'w' || *p == 'W') && (p[1] == 'b' || p[1] == 'B'))
                        p += 2;
                    else
                        break;
                }
                if (*p == 'i' || *p == 'I' || *p == 'j' || *p == 'J') {
                    is_imag = true;
                    p++;
                }
            }
            if (is_float) {
                cur = cur->next = new_token(TK_FNUM, q, p, cur_lineno);
                if (is_imag)
                    cur->val = fkind | 4;
                else
                    cur->val = fkind;
                // Strip C23 digit separators before calling libc
                char fbuf[128];
                int flen = 0;
                for (char *rp = q; rp < p && flen < 127; rp++)
                    if (*rp != '\'') fbuf[flen++] = *rp;
                fbuf[flen] = '\0';
                if (fkind == 1)
                    cur->fval = (double)strtof(fbuf, NULL);
                else
                    cur->fval = strtod(fbuf, NULL);
            } else if (is_imag) {
                cur = cur->next = new_token(TK_FNUM, q, p, cur_lineno);
                cur->val = 4 | 8;
                if (q[0] == '0' && (q[1] == 'b' || q[1] == 'B')) {
                    int64_t iv = 0;
                    char *bp = q + 2;
                    while (*bp == '0' || *bp == '1' || *bp == '\'') {
                        if (*bp != '\'') iv = iv * 2 + (*bp - '0');
                        bp++;
                    }
                    cur->fval = (double)iv;
                } else if (q[0] == '0' && (q[1] == 'o' || q[1] == 'O')) {
                    int64_t iv = 0;
                    for (char *rp = q + 2; rp < p; rp++)
                        if (*rp >= '0' && *rp <= '7') iv = iv * 8 + (*rp - '0');
                    cur->fval = (double)iv;
                } else if (q[0] == '0' && (isdigit(q[1]) || (dsep && q[1] == '\'' && isdigit(q[2])))) {
                    int64_t iv = 0;
                    for (char *rp = q + 1; rp < p; rp++)
                        if (*rp >= '0' && *rp <= '7') iv = iv * 8 + (*rp - '0');
                    cur->fval = (double)iv;
                }
            } else {
                cur = cur->next = new_token(TK_NUM, q, p, cur_lineno);
                if (q[0] == '0' && (q[1] == 'b' || q[1] == 'B')) {
                    uint64_t val = 0;
                    char *bp = q + 2;
                    while (*bp == '0' || *bp == '1' || *bp == '\'') {
                        if (*bp != '\'') val = val * 2 + (uint64_t)(*bp - '0');
                        bp++;
                    }
                    cur->val = (int64_t)val;
                } else if (q[0] == '0' && (q[1] == 'x' || q[1] == 'X')) {
                    uint64_t val = 0;
                    for (char *rp = q + 2; rp < p && *rp != 'u' && *rp != 'U' && *rp != 'l' && *rp != 'L'; rp++)
                        if (*rp != '\'') val = val * 16 + (uint64_t)(isdigit(*rp) ? *rp - '0' : ((*rp | 32) - 'a' + 10));
                    cur->val = (int64_t)val;
                } else if (q[0] == '0' && (q[1] == 'o' || q[1] == 'O')) {
                    uint64_t val = 0;
                    for (char *rp = q + 2; rp < p && *rp != 'u' && *rp != 'U' && *rp != 'l' && *rp != 'L'; rp++)
                        if (*rp != '\'' && *rp >= '0' && *rp <= '7') val = val * 8 + (uint64_t)(*rp - '0');
                    cur->val = (int64_t)val;
                } else if (q[0] == '0' && (isdigit(q[1]) || (dsep && q[1] == '\'' && isdigit(q[2])))) {
                    // Octal: leading 0 followed by digit (not x/b),
                    // possibly with a digit separator right after the 0
                    uint64_t val = 0;
                    for (char *rp = q + 1; rp < p && *rp != 'u' && *rp != 'U' && *rp != 'l' && *rp != 'L'; rp++)
                        if (*rp != '\'' && *rp >= '0' && *rp <= '7') val = val * 8 + (uint64_t)(*rp - '0');
                    cur->val = (int64_t)val;
                } else {
                    uint64_t val = 0;
                    for (char *rp = q; rp < p && *rp != 'u' && *rp != 'U' && *rp != 'l' && *rp != 'L'; rp++)
                        if (isdigit(*rp)) val = val * 10 + (uint64_t)(*rp - '0');
                    cur->val = (int64_t)val;
                }
            }
            cur->len = (int)(p - q);
            continue;
        }
        if ((unsigned char)*p >= 128) {
            char *start = p;
            char *pos;
            uint32_t c = decode_utf8(&pos, p);
            if (!is32_ident1(c) || pos == p) // ensure utf-8 progress
                continue;
            do {
                p = pos;
                c = decode_utf8(&pos, p);
            } while (is_ident2(*p) || (is32_ident2(c) && pos != p));
            if (pos != p) {
                cur = cur->next = new_token(TK_IDENT, start, p, cur_lineno);
                int kw = keyword_id(start, p - start, NULL);
                if (kw != ID_NONE) {
                    cur->kw = kw;
                    cur->name = kw_canon[kw];
                } else {
                    cur->name = str_intern(start, p - start);
                }
                if (!opt_Wno_homoglyph) {
                    const char *w = u8ident_check_ident_align16(cur->name, p - start);
                    if (w)
                        warn_tok(cur, "%s", w);
                }
            }
            continue;
        }
        if (is_ident1(*p)) {
            // Don't tokenize L/u/U as identifiers if followed by string/char literal
            bool std_c23 = opt_std_version && strcmp(opt_std_version, "202311L") >= 0;
            if ((*p == 'L' || *p == 'U') && (p[1] == '"' || p[1] == '\'')) {
                // Fall through to string/char literal handling
            } else if (*p == 'u' && (p[1] == '"' || p[1] == '\'' || (p[1] == '8' && p[2] == '"') ||
                                     // u8'' character constants are C23-only; in
                                     // older modes u8 is an ordinary identifier.
                                     (p[1] == '8' && p[2] == '\'' && std_c23))) {
                // Fall through to string/char literal handling
            } else {
                char *start = p;
                char *pos = p + 1;
                uint32_t c;
                do {
                    p = pos;
                    c = decode_utf8(&pos, p);
                } while (is_ident2(*p) || (is32_ident2(c) && pos != p));
                cur = cur->next = new_token(TK_IDENT, start, p, cur_lineno);
                int kw = keyword_id(start, p - start, NULL);
                if (kw != ID_NONE) {
                    cur->kw = kw;
                    cur->name = kw_canon[kw];
                } else {
                    cur->name = str_intern(start, p - start);
                }
                if (!opt_Wno_homoglyph) {
                    const char *w = u8ident_check_ident_align16(cur->name, p - start);
                    if (w)
                        warn_tok(cur, "%s", w);
                }
                continue;
            }
        }

        // String literal
        int prefix = 0;
        if (p[0] == 'u' && p[1] == '8' && p[2] == '"') {
            prefix = '8'; // u8 prefix => char8_t
            p += 2;
        } else if ((*p == 'L' || *p == 'u' || *p == 'U') && p[1] == '"') {
            prefix = *p;
            p++;
        }

        if (*p == '"') {
            // Include the prefix in the start position.  prefix holds the
            // prefix *character* ('8' for u8, else 'L'/'u'/'U'), not a
            // length, so map it back to how many bytes p was advanced.
            char *start = p - (prefix ? (prefix == '8' ? 2 : 1) : 0);
            p++;
            char *buf = arena_alloc(2048); // Pre-allocate scratch buffer
            int len = 0;

            while (*p && *p != '"' && *p != '\n') {
                if (*p == '\\') {
                    p++;
                    uint32_t val = 0;
                    bool have_cp = false;
                    if (*p == 'u' || *p == 'U') {
                        int n_digits = (*p == 'u') ? 4 : 8;
                        p++;
                        for (int i = 0; i < n_digits; i++) {
                            int digit = from_hex(*p);
                            if (digit < 0) error_at(p, "invalid unicode escape");
                            val = val * 16 + digit;
                            p++;
                        }
                        have_cp = true;
                    } else if (prefix && prefix != '8' &&
                               (*p == 'x' || ('0' <= *p && *p <= '7'))) {
                        // In L/u/U strings a numeric escape denotes a wide
                        // character value, not a byte. The buffer holds
                        // UTF-8 that the wide-string converter decodes, so
                        // encode the value; a raw byte >= 0x80 would be an
                        // invalid UTF-8 sequence there.
                        if (*p == 'x') {
                            p++;
                            int digit = from_hex(*p);
                            if (digit < 0)
                                error_at(p, "invalid hex escape");
                            while ((digit = from_hex(*p)) >= 0) {
                                val = val * 16 + digit;
                                p++;
                            }
                        } else {
                            int n = 0;
                            while (n < 3 && '0' <= *p && *p <= '7') {
                                val = val * 8 + (*p - '0');
                                p++;
                                n++;
                            }
                        }
                        have_cp = true;
                    } else {
                        buf[len++] = read_escaped_char(&p, p);
                    }
                    if (have_cp) {
                        // Encode as UTF-8
                        if (val < 0x80) {
                            buf[len++] = (char)val;
                        } else if (val < 0x800) {
                            buf[len++] = 0xC0 | (val >> 6);
                            buf[len++] = 0x80 | (val & 0x3F);
                        } else if (val < 0x10000) {
                            buf[len++] = 0xE0 | (val >> 12);
                            buf[len++] = 0x80 | ((val >> 6) & 0x3F);
                            buf[len++] = 0x80 | (val & 0x3F);
                        } else {
                            buf[len++] = 0xF0 | (val >> 18);
                            buf[len++] = 0x80 | ((val >> 12) & 0x3F);
                            buf[len++] = 0x80 | ((val >> 6) & 0x3F);
                            buf[len++] = 0x80 | (val & 0x3F);
                        }
                    }
                } else {
                    char sc = *p++;
                    if (!prefix && opt_exec_charset && (unsigned char)sc < 0x80)
                        sc = (char)to_exec_charset((uint8_t)sc);
                    buf[len++] = sc;
                }
            }
            if (*p != '"') error_at(start, "unclosed string literal");
            p++;

            buf[len] = '\0';
            cur = cur->next = new_token(TK_STR, start, p, cur_lineno);
            cur->str = str_intern(buf, len); // intern it
            // len stays the decoded content length (the parser relies on
            // it); the raw source span (prefix..closing quote) goes in val
            // so the preprocessor can recover the exact spelling.
            cur->len = len;
            cur->val = (int64_t)(p - start);
            cur->string_literal_prefix = prefix;
            continue;
        }

        // Character literal
        int char_prefix = 0;
        if (p[0] == 'u' && p[1] == '8' && p[2] == '\'') {
            char_prefix = '8';
            p += 2;
        } else if ((*p == 'L' || *p == 'u' || *p == 'U') && p[1] == '\'') {
            char_prefix = *p;
            p++;
        }

        if (*p == '\'') {
            char *start = p;
            p++;
            uint32_t cval = 0;
            while (*p && *p != '\'' && *p != '\n') {
                if (*p == '\\') {
                    p++;
                    if (char_prefix && (*p == 'u' || *p == 'U')) {
                        // Universal character name: full codepoint value
                        int n_digits = (*p == 'u') ? 4 : 8;
                        p++;
                        cval = 0;
                        for (int i = 0; i < n_digits; i++) {
                            int digit = from_hex(*p);
                            if (digit < 0) error_at(p, "invalid unicode escape");
                            cval = cval * 16 + digit;
                            p++;
                        }
                    } else if (char_prefix && char_prefix != '8' &&
                               (*p == 'x' || ('0' <= *p && *p <= '7'))) {
                        // Wide char literal: numeric escape is a full wide
                        // character value; (uint8_t) would truncate L'\x2219'
                        cval = 0;
                        if (*p == 'x') {
                            p++;
                            int digit = from_hex(*p);
                            if (digit < 0)
                                error_at(p, "invalid hex escape");
                            while ((digit = from_hex(*p)) >= 0) {
                                cval = cval * 16 + digit;
                                p++;
                            }
                        } else {
                            int n = 0;
                            while (n < 3 && '0' <= *p && *p <= '7') {
                                cval = cval * 8 + (*p - '0');
                                p++;
                                n++;
                            }
                        }
                    } else {
                        cval = (uint8_t)read_escaped_char(&p, p);
                    }
                } else if (char_prefix && (unsigned char)*p >= 0x80) {
                    // Wide char: decode full UTF-8 codepoint
                    cval = decode_utf8(&p, p);
                } else {
                    cval = (uint8_t)*p++;
                    if (!char_prefix)
                        cval = to_exec_charset(cval);
                }
            }
            if (*p != '\'') error_at(start, "unclosed character literal");
            p++;
            cur = cur->next = new_token(TK_NUM, start, p, cur_lineno);
            // Plain char literals have type int with the value of a
            // (signed) char: '\xe2' is -30, not 226. Prefixed literals
            // (L/u/U/u8) hold non-negative wide/unsigned values.
            cur->val = char_prefix ? (int64_t)cval : (int64_t)(int8_t)cval;
            // Record the prefix so the parser can type the constant
            // (u8 -> unsigned char, u -> char16_t, U -> char32_t).
            cur->string_literal_prefix = char_prefix;
            continue;
        }

        // Punctuators
        int punct_len = read_punct(p);
        if (punct_len) {
            cur = cur->next = new_token(TK_PUNCT, p, p + punct_len, cur_lineno);
            p += punct_len;
            continue;
        }

        error_at(p, "invalid token");
    }

    *pp = p;
    *plineno = cur_lineno;
    return head.next;
}

// Tokenize a given string and return new tokens.
// Used for internally generated sources (parser builtins); user code reaches
// the parser through the token-based preprocessor (preprocess()).
Token *tokenize(char *filename, char *p) {
    lex_pp_mode = false;
    current_input = p;
    current_filename = filename;
    /* Reset #line-directive tracking from any previous tokenize() call in
     * this process (e.g. rcc_lib compiling multiple files in-process);
     * otherwise a leftover current_line_offset/line_num from the previous
     * file's buffer produces bogus line numbers in warn_tok()/error_at()
     * for files with no #line directive of their own. */
    current_line_offset = 0;
    line_num = 1;
    current_debug_filename = filename;
    Token head = {};
    Token *cur = &head;
    int cur_lineno = 1;

    for (Token *tk; (tk = lex_one(&p, &cur_lineno)) != NULL;)
        cur = cur->next = tk;

    cur->next = new_token(TK_EOF, p, p, cur_lineno);
    return head.next;
}
