// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from chibicc by Rui Ueyama.
#include "rcc.h"
#include <stdarg.h>
#include <ctype.h>

// Input string
char *current_input;
char *current_filename;
int current_line_offset = 0;
int line_num = 1;
// Tracks the filename from #line directives for debug info (tok->filename).
// Kept separate from current_filename so error messages always use the
// original in_path (relative/short) while tok->filename gets the resolved path.
static char *current_debug_filename;

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
// TODO not-returning attribute
static void verror_at(char *loc, int len, char *fmt, va_list ap) {
    if (!loc) {
        fprintf(stderr, "\033[1;31merror:\033[0m ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        exit(1);
    }
    // Find line containing `loc`
    char *line = loc;
    while (current_input < line && line[-1] != '\n')
        line--;

    char *end = loc;
    while (*end != '\n' && *end != '\0')
        end++;

    int reported_line = compute_line_no(loc);

    // Print filename and line info
    fprintf(stderr, "\033[1;37m%s:%d: \033[0m", current_filename, reported_line);
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
    verror_at(loc, 1, fmt, ap);
    exit(1);
}

// Like error_tok but prints filename:line: error: message only (no source context).
// Used for inline-asm validation errors so output matches TCC's assembler format.
// cppcheck-suppress va_end_missing
__attribute__((noreturn)) void error_tok_simple(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int lineno = tok && tok->ptr ? compute_line_no(tok->ptr) : 1;
    fprintf(stderr, "%s:%d: error: ",
            current_filename ? current_filename : "<unknown>", lineno);
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
        verror_at(NULL, 0, fmt, ap);
        exit(1);
    }
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->ptr, tok->len, fmt, ap);
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
    char *line = tok->ptr;
    while (current_input < line && line[-1] != '\n')
        line--;
    int reported_line = line_num;
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
    // Use basename of file
    const char *base = current_filename;
    for (const char *p = current_filename; *p; p++)
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
    if (opt_g) {
        tok->filename = current_debug_filename;
        tok->lineno = lineno;
    }
    return tok;
}

static bool startswith(char *p, char *q) {
    return strncmp(p, q, strlen(q)) == 0;
}

// Read a punctuator token from p and returns its length.
static int read_punct(char *p) {
    if (startswith(p, "..."))
        return 3;
    if (startswith(p, "<<=") || startswith(p, ">>="))
        return 3;
    if (startswith(p, "==") || startswith(p, "!=") ||
        startswith(p, "<=") || startswith(p, ">=") ||
        startswith(p, "++") || startswith(p, "--") ||
        startswith(p, "&&") || startswith(p, "||") ||
        startswith(p, "->") || startswith(p, "<<") ||
        startswith(p, ">>") || startswith(p, "+=") ||
        startswith(p, "-=") || startswith(p, "*=") ||
        startswith(p, "/=") || startswith(p, "%=") ||
        startswith(p, "&=") || startswith(p, "|=") ||
        startswith(p, "^="))
        return 2;

    return ispunct(*p) ? 1 : 0;
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

// Tokenize a given string and return new tokens.
// Note: Input should already be preprocessed by the caller.
Token *tokenize(char *filename, char *p) {
    current_input = p;
    current_filename = filename;
    /* Reset #line-directive tracking from any previous tokenize() call in
     * this process (e.g. rcc_lib compiling multiple files in-process);
     * otherwise a leftover current_line_offset/line_num from the previous
     * file's buffer produces bogus line numbers in warn_tok()/error_at()
     * for files with no #line directive of their own. */
    current_line_offset = 0;
    line_num = 1;
    if (opt_g)
        current_debug_filename = filename;
    Token head = {};
    Token *cur = &head;
    int cur_lineno = 1;

    while (*p) {
        // Skip whitespace characters.
        if (isspace(*p)) {
            if (*p == '\n') cur_lineno++;
            p++;
            continue;
        }

        // Skip preprocessor directives naively
        if (*p == '#') {
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
                        if (opt_g)
                            current_debug_filename = str_intern(fn_start, q - fn_start);
                        current_input = p;
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
            for (char *r = p + 2; r < q; r++)
                if (*r == '\n') cur_lineno++;
            p = q + 2;
            continue;
        }

        // Numeric literal (integer or floating point)
        if (isdigit(*p) || (*p == '.' && isdigit(p[1]))) {
            char *q = p;
            bool is_float = false;

            if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
                p += 2;
                while (isxdigit(*p) || *p == '\'') p++;
                if (*p == '.') {
                    is_float = true;
                    p++;
                    while (isxdigit(*p) || *p == '\'') p++;
                }
                if (*p == 'p' || *p == 'P') {
                    is_float = true;
                    p++;
                    if (*p == '+' || *p == '-') p++;
                    while (isdigit(*p) || *p == '\'') p++;
                }
            } else if (*p == '0' && (p[1] == 'b' || p[1] == 'B')) {
                p += 2;
                while (*p == '0' || *p == '1' || *p == '\'') p++;
            } else {
                while (isdigit(*p) || *p == '\'') p++;
                if (*p == '.' && p[1] != '.') {
                    is_float = true;
                    p++;
                    while (isdigit(*p) || *p == '\'') p++;
                }
                if (*p == 'e' || *p == 'E') {
                    is_float = true;
                    p++;
                    if (*p == '+' || *p == '-') p++;
                    while (isdigit(*p) || *p == '\'') p++;
                }
            }

            // Check for float/imaginary suffix: f/F, l/L, i/I in any order
            int fkind = 0; // 0=double, 1=float, 2=long double
            bool is_imag = false;
            if (is_float) {
                // _FloatN suffixes (C23): F32, F64, F128, F32x, F64x (also lowercase)
                // Must be checked before the plain f/F suffix consumer.
                if ((*p == 'f' || *p == 'F') && isdigit(p[1])) {
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
                    if (*p == 'i' || *p == 'I') {
                        is_imag = true;
                        p++;
                    }
                } else {
                    for (int pass = 0; pass < 2; pass++) {
                        if (*p == 'f' || *p == 'F') {
                            fkind = 1;
                            p++;
                        } else if (*p == 'l' || *p == 'L') {
                            if (is_float) {
                                fkind = 2;
                                p++;
                            } else
                                break;
                        } else if (*p == 'i' || *p == 'I') {
                            is_imag = true;
                            p++;
                        }
                    }
                    // Also handle DD/DF/DL decimal suffixes
                    if (fkind == 0 && !is_imag &&
                        (*p == 'd' || *p == 'D') &&
                        (p[1] == 'd' || p[1] == 'D' || p[1] == 'f' || p[1] == 'F' || p[1] == 'l' || p[1] == 'L')) {
                        if (p[1] == 'd' || p[1] == 'D') fkind = 0;
                        else if (p[1] == 'f' || p[1] == 'F')
                            fkind = 1;
                        else
                            fkind = 2;
                        p += 2;
                    }
                } // end else (not _FloatN suffix)
            } else {
                // Integer suffix (including imaginary i/I extension)
                while (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L')
                    p++;
                if (*p == 'i' || *p == 'I') {
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
                } else {
                    int64_t iv = 0;
                    for (char *rp = q; rp < p; rp++)
                        if (isdigit(*rp)) iv = iv * 10 + (*rp - '0');
                    cur->fval = (double)iv;
                }
            } else {
                cur = cur->next = new_token(TK_NUM, q, p, cur_lineno);
                if (q[0] == '0' && (q[1] == 'b' || q[1] == 'B')) {
                    int64_t val = 0;
                    char *bp = q + 2;
                    while (*bp == '0' || *bp == '1' || *bp == '\'') {
                        if (*bp != '\'') val = val * 2 + (*bp - '0');
                        bp++;
                    }
                    cur->val = val;
                } else if (q[0] == '0' && (q[1] == 'x' || q[1] == 'X')) {
                    int64_t val = 0;
                    for (char *rp = q + 2; rp < p && *rp != 'u' && *rp != 'U' && *rp != 'l' && *rp != 'L'; rp++)
                        if (*rp != '\'') val = val * 16 + (isdigit(*rp) ? *rp - '0' : ((*rp | 32) - 'a' + 10));
                    cur->val = val;
                } else {
                    int64_t val = 0;
                    for (char *rp = q; rp < p && *rp != 'u' && *rp != 'U' && *rp != 'l' && *rp != 'L'; rp++)
                        if (isdigit(*rp)) val = val * 10 + (*rp - '0');
                    cur->val = val;
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
            if ((*p == 'L' || *p == 'u' || *p == 'U') && (p[1] == '"' || p[1] == '\'')) {
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
        if ((*p == 'L' || *p == 'u' || *p == 'U') && p[1] == '"') {
            prefix = *p;
            p++;
        }

        if (*p == '"') {
            char *start = p - prefix; // Include prefix in start position
            p++;
            char *buf = arena_alloc(2048); // Pre-allocate scratch buffer
            int len = 0;

            for (;;) {
                while (*p && *p != '"' && *p != '\n') {
                    if (*p == '\\') {
                        p++;
                        if (*p == 'u' || *p == 'U') {
                            int n_digits = (*p == 'u') ? 4 : 8;
                            p++;
                            uint32_t val = 0;
                            for (int i = 0; i < n_digits; i++) {
                                int digit = from_hex(*p);
                                if (digit < 0) error_at(p, "invalid unicode escape");
                                val = val * 16 + digit;
                                p++;
                            }
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
                        } else {
                            buf[len++] = read_escaped_char(&p, p);
                        }
                    } else {
                        buf[len++] = *p++;
                    }
                }
                if (*p != '"') error_at(start, "unclosed string literal");
                p++;

                // String concatenation: merge adjacent string literals
                char *q = p;
                while (isspace(*q)) {
                    if (*q == '\n') cur_lineno++;
                    q++;
                }
                if ((*q == 'L' || *q == 'u' || *q == 'U') && q[1] == '"')
                    q++;
                if (*q != '"')
                    break;
                p = q + 1;
            }

            buf[len] = '\0';
            cur = cur->next = new_token(TK_STR, start, p, cur_lineno);
            cur->str = str_intern(buf, len); // intern it
            cur->len = len;
            cur->string_literal_prefix = prefix;
            continue;
        }

        // Character literal
        int char_prefix = 0;
        if ((*p == 'L' || *p == 'u' || *p == 'U') && p[1] == '\'') {
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
                    cval = (uint8_t)read_escaped_char(&p, p);
                } else if (char_prefix && (unsigned char)*p >= 0x80) {
                    // Wide char: decode full UTF-8 codepoint
                    cval = decode_utf8(&p, p);
                } else {
                    cval = (uint8_t)*p++;
                }
            }
            if (*p != '\'') error_at(start, "unclosed character literal");
            p++;
            cur = cur->next = new_token(TK_NUM, start, p, cur_lineno);
            cur->val = cval;
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

    cur->next = new_token(TK_EOF, p, p, cur_lineno);
    return head.next;
}
