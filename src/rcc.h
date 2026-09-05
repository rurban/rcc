// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from chibicc by Rui Ueyama.
#ifndef RCC_H
#define RCC_H

// Auto-detect target architecture from host
#if defined(__aarch64__) && !defined(ARCH_ARM64)
#define ARCH_ARM64
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <float.h>
#include <setjmp.h>

#ifndef VERSION
#define VERSION "v1.2-dev"
#endif

//
// Tokenizer / Lexer
//

#include "keyword_ids.h"

typedef enum {
    TK_IDENT, // Identifiers
    TK_PUNCT, // Punctuators
    TK_NUM, // Numeric literals
    TK_FNUM, // Floating-point literals
    TK_STR, // String literals
    TK_EOF, // End of file
    TK_NL, // Newline (preprocessor-internal, never reaches the parser)
    TK_CNL, // Newline inside a block comment (preprocessor-internal)
} TokenKind;

// Token type
typedef struct Token Token;
// C99 6.10.3.4p2 hide set: linked list of macro names whose expansions a
// token has flowed through; the token must never be re-expanded as one of
// these. Nodes are arena-allocated and immutable (shared across tokens).
struct Hideset {
    struct Macro *name;
    struct Hideset *next;
};

struct Token {
    TokenKind kind; // Token kind
    Token *next; // Next token
    int64_t val; // If kind is TK_NUM, its value
    double fval; // If kind is TK_FNUM, its value
    char *name; // If kind is TK_IDENT, its name
    int kw; // keyword/builtin ID if identifier is a known word, else ID_NONE
    char *str; // If kind is TK_STR, its contents
    char *ptr; // buffer location
    int len; // Token length
    // For string literals: 0 = regular, 'L' = wide, 'u' = char16_t, 'U' = char32_t
    int string_literal_prefix;
    char *filename; // Source file name (after #line substitution)
    int lineno; // Source line number (after #line substitution)
    // Set on a ## paste result whose macro-body definition had the next
    // token immediately adjacent (no whitespace) — lets #-stringize keep
    // them touching even though the pasted token's spelling now lives in a
    // freshly lexed buffer with no real adjacency to the following token's
    // original source pointer.
    bool no_space_after;
    // ## paste result: freshly lexed, must NOT inherit the frame's hide
    // set when pulled (C99 6.10.3.4p2 pastes are not subject to the
    // surrounding macro's paint — the operands' own hidesets are the only
    // ones that carry over). Otherwise the pasted token accumulates every
    // ancestor's paint and can end up painted with its own name,
    // self-blocking (tcc's 64_macro_nesting: CAT(A,B) pastes AB, which is
    // then painted {AB, CAT, CAT2} and never expands as AB(x)).
    bool no_paint;
    // C99 6.10.3.4p2 "blue paint" / hide set: the macros whose expansions
    // produced this token (every ancestor replacement list the token
    // flowed through, not just the innermost producer). The token is never
    // re-expanded as any of those macro names. A single-name paint is not
    // enough: it cannot stop the A<->B mutual-recursion ping-pong
    // (glibc's `#define alloca(x) __builtin_alloca(x)` vs rcc's own
    // `define_pre("__builtin_alloca", "alloca")` alias loops forever,
    // stopping only at the nframes cap with whichever name the parity
    // happened to land on), and metalang99's eval machine needs a name
    // re-invocable from a NESTED replacement even while an older frame of
    // the same name is still active — the union of paints (hide set) is
    // exactly gcc's semantics: a token stops only when its own name is
    // already in its set, which happens after one full recursion cycle.
    struct Hideset *blue;
};

// Error reporting
__attribute__((noreturn)) void error(char *fmt, ...);
__attribute__((noreturn)) void error_at(char *loc, char *fmt, ...);
__attribute__((noreturn)) void error_tok(Token *tok, char *fmt, ...);
__attribute__((noreturn)) void error_tok_simple(Token *tok, char *fmt, ...);
void warn_tok(Token *tok, char *fmt, ...);

// Error collection (GH #34): parse errors are counted and recovered from
// (longjmp back into parse()'s top-level loop) instead of exiting, unless
// -Wfatal-errors. Compilation stops early after opt_fmax_errors errors.
extern int error_count;
extern bool opt_Wfatal_errors;
extern int opt_fmax_errors; // 0 = unlimited
extern jmp_buf error_recovery_jmp;
extern bool error_recovery_active;
extern Token *error_recovery_tok; // token to resynchronize from
extern jmp_buf stmt_recovery_jmp; // statement-level recovery (finer-grained)
extern bool stmt_recovery_active;

// Lexer state (for token injection)
extern char *current_input;
extern char *current_filename;
extern int current_line_offset;
extern int line_num;
extern char *current_debug_filename;

// Allocator / Utils
void *arena_alloc(size_t size);
void *scratch_alloc(size_t size); // resettable arena for transient PP expansion strings
void scratch_reset(void); // rewind scratch arena (all prior scratch pointers die)
char *format(char *fmt, ...);
char *str_intern(const char *start, int len);
void str_intern_resize(size_t src_bytes); // call after read_file(), before preprocess()
char *path_basename(char *path);

// Lexer entry points
// preprocess() is the single scanner: it lexes each source file once and
// runs macro expansion on the token stream, returning parser-ready tokens.
Token *preprocess(char *filename, char *p);
void pp_print_tokens(Token *tok, FILE *out); // -E: re-create the lines from the tokens
char *pp_tokens_to_text(Token *tok); // like pp_print_tokens but into a heap buffer
char *dump_macros_text(void); // -dM
Token *lex_one(char **pp, int *plineno);
extern bool lex_pp_mode;
// Set while preprocessing a ".S"/".s" input (assembler-with-cpp mode): a '#'
// that isn't the first token on its line is GAS's end-of-line comment
// marker, not preprocessor punctuation — see lex_one()'s use of it.
extern bool lex_asm_cpp_mode;
void register_source_buffer(char *start, char *end);
void add_define(char *def);
void add_undef(char *name);
void remove_cmdline_define(const char *name);
void add_include_path(const char *path);
void add_quote_include_path(const char *path); // -iquote: quote-form ("...") includes only
// Default ELF symbol visibility applied to globals/functions without an
// explicit __attribute__((visibility(...))). Set by -fvisibility=hidden
// etc.; STV_DEFAULT unless overridden. (obj.h's STV_* values.)
extern uint8_t rcc_default_visibility;
// -nostdinc: skip system include paths
extern bool opt_nostdinc;
// Make dependency generation (-Wp,-MMD, / -MD / -MMD / -MF / -MT / -MQ / -MP
// / -M / -MM)
extern const char *opt_depfile;
extern const char *opt_dep_target;
extern bool opt_gen_deps;
extern bool opt_dep_phony;
// -M/-MM: dependency-rule-only mode, no compilation at all (see main.c)
extern bool opt_deps_only;
// -fmacro-prefix-map=old=new
extern const char *opt_prefix_map_old;
extern const char *opt_prefix_map_new;
// -include <file>: pre-include before main source
void add_preinclude(const char *path);
void add_prefix_map(const char *old, const char *new_str);
// Write dependency file after preprocessing
void write_dep_file(const char *out_path, const char *main_fpath);
// -M/-MM: print the dependency rule directly to an open stream
void print_dep_rule(FILE *f, const char *out_path, const char *main_fpath);
void rcc_reset_state(void);
void print_search_dirs(const char *gcc);
Token *tokenize(char *filename, char *p);
void init_builtins(void);

// Pre-interned name pointers for O(1) pointer-equality matching.
// All identifiers from the lexer are str_intern'd, so comparing
// against these interned pointers avoids strcmp.
void init_builtin_names(void);
extern char *bi_bswap16, *bi_bswap32, *bi_bswap64;
extern char *bi_clz, *bi_clzl, *bi_clzll;
extern char *bi_ctz, *bi_ctzl, *bi_ctzll;
extern char *bi_popcount, *bi_popcountl, *bi_popcountll;
extern char *bi_parity, *bi_parityl, *bi_parityll;
extern char *bi_clrsb, *bi_clrsbl, *bi_clrsbll;
extern char *bi_ffs, *bi_ffsl, *bi_ffsll;
extern char *bi_prefetch, *bi_frame_address, *bi_return_address;
extern char *bi_setjmp, *bi_longjmp;
extern char *bi_signbit, *bi_signbitf, *bi_signbitl;
extern char *bi_isinf, *bi_isinff, *bi_isinfl;
extern char *bi_isfinite, *bi_isfinitef, *bi_isfinitel;
extern char *bi_isnormal, *bi_isnormalf, *bi_isnormall;
extern char *bi_fpclassify, *bi_fpclassifyf, *bi_fpclassifyl;
extern char *bi_copysign, *bi_copysignf, *bi_copysignl;
extern char *bi_fma, *bi_fmaf, *bi_fmal;
extern char *bi_abs, *bi_labs, *bi_llabs;
extern char *bi_add_overflow, *bi_sub_overflow;
extern char *bi_add_overflow_p, *bi_sub_overflow_p;
extern char *bi_mul_overflow, *bi_mul_overflow_p;
extern char *bi_sadd_overflow, *bi_saddl_overflow, *bi_saddll_overflow;
extern char *bi_uadd_overflow, *bi_uaddl_overflow, *bi_uaddll_overflow;
extern char *bi_ssub_overflow, *bi_ssubl_overflow, *bi_ssubll_overflow;
extern char *bi_usub_overflow, *bi_usubl_overflow, *bi_usubll_overflow;
extern char *bi_smul_overflow, *bi_smull_overflow, *bi_smulll_overflow;
extern char *bi_umul_overflow, *bi_umull_overflow, *bi_umulll_overflow;
extern char *bi_memset, *bi_memcpy, *bi_memcmp;
extern char *bi_strlen, *bi_strcmp, *bi_strchr;
extern char *bi_unreachable;
extern char *bi_s_abs, *bi_s_labs, *bi_s_llabs;
extern char *bi_s_memset, *bi_s_memcpy, *bi_s_memcmp;
extern char *bi_s_strlen, *bi_s_strcmp, *bi_s_strchr;
extern char *bi_s_printf, *bi_s_fprintf, *bi_s_vprintf, *bi_s_vfprintf;
extern char *bi_s_puts, *bi_s_fputs;
extern char *bi_s_sprintf, *bi_s_snprintf;
extern char *bi_s_scanf, *bi_s_fscanf, *bi_s_sscanf;
extern char *bi_s_alloca, *bi_s_builtin_alloca;
extern char *bi_chk_printf, *bi_chk_vprintf;
extern char *bi_chk_fprintf, *bi_chk_vfprintf;
extern char *bi_sqrtps, *bi_sqrtss, *bi_rsqrtps;


//
// Parser
//

// Type System
typedef enum { QUAL_CONST = 1,
               QUAL_VOLATILE = 2,
               QUAL_RESTRICT = 4,
               QUAL_ATOMIC = 8 } TypeQual;

typedef enum {
    MEMORDER_RELAXED = 0,
    MEMORDER_CONSUME = 1,
    MEMORDER_ACQUIRE = 2,
    MEMORDER_RELEASE = 3,
    MEMORDER_ACQ_REL = 4,
    MEMORDER_SEQ_CST = 5,
} MemoryOrder;

typedef enum {
    TY_VOID,
    TY_BOOL,
    TY_INT,
    TY_CHAR,
    TY_SHORT,
    TY_LONG,
    TY_LLONG,
    TY_INT128,
    TY_FLOAT,
    TY_DOUBLE,
    TY_LDOUBLE,
    TY_COMPLEX, // _Complex
    TY_PTR,
    TY_ARRAY,
    TY_VLA, // variable-length array
    TY_STRUCT,
    TY_UNION,
    TY_FUNC,
    TY_NULLPTR_T, // C23 nullptr_t
    TY_BITINT, // C23 _BitInt(N) / unsigned _BitInt(N)
    TY_DECIMAL32, // C23 _Decimal32 (7 digits, BID 32-bit)
    TY_DECIMAL64, // C23 _Decimal64 (16 digits, BID 64-bit)
    TY_DECIMAL128, // C23 _Decimal128 (34 digits, BID 128-bit)
} TypeKind;

typedef struct Node Node;
typedef struct Type Type;
typedef struct Member Member;
typedef struct Contract Contract;

// A single `pre(EXPR)` / `post([NAME:] EXPR)` contract-specifier trailing
// a function declarator's parameter list (Gustedt's "Contracts for C":
// https://gustedt.wordpress.com/2025/03/10/contracts-for-c/, minus the
// pre()/post() *statement* forms — see parse_contract_specs() in
// parser.c). Deliberately unresolved at parse time: `cond_start`/
// `cond_end` bound the raw, unparsed condition tokens, replayed (via
// conditional()) once per point of use against that use's own real
// parameter/return-binding locals — see activate_function_contracts()
// and apply_postconds_to_return() in parser.c.
struct Contract {
    Contract *next;
    Token *tok; // the 'pre'/'post' keyword token, for diagnostics
    Token *cond_start; // first token of the condition
    Token *cond_end; // the matching ')' (condition tokens are [cond_start, cond_end))
    char *bind_name; // post(NAME: ...) return-value binding, or NULL
};

enum {
    BF_MODE_DEFAULT,
    BF_MODE_GCC,
    BF_MODE_MS,
};

struct Member {
    Member *next;
    Type *ty;
    char *name;
    int offset;
    // Non-NULL for members that follow a variable-size (VLA) member within a
    // VLA-containing struct: a runtime expression for this member's byte
    // offset, evaluated and added to the base address instead of `offset`.
    Node *offset_expr;
    int bit_width; // 0 = not a bitfield
    int bit_offset; // bit position within the storage unit
    int bf_load_size; // effective R/W size for dense-packed bitfields (0 = use ty->size)
    Token *tok; // declaration site, for diagnostics (may be NULL)
};

struct Type {
    TypeKind kind;
    int64_t size; // sizeof
    int align; // alignment
    bool is_unsigned;
    bool is_enum; // enum type — treated as unsigned for bitfield extraction
    bool is_enum_fixed; // C23 enum with fixed underlying type
    // Identity anchor for two distinct `enum` declarations that happen to
    // share size/signedness/kind (e.g. two int-sized enums, or a fixed-
    // underlying-type enum matching its own underlying type by chance).
    // Set to the completed enum Type's own address, and preserved across
    // the plain-struct copies used for bare `enum tag` type-name lookups,
    // so __builtin_types_compatible_p can tell "same enum" from "same
    // representation" — plain (non-enum) types leave this NULL.
    Type *enum_id;
    // Identity anchor for two distinct struct/union declarations sharing
    // the same tag: an INCOMPLETE struct/union's qualified variants
    // (Type.qual_variants, see qualify_struct_type() in parser.c) are
    // fresh Type objects at every `const struct S *`-style use site while
    // S stays forward-declared, so pointer identity (a == b) and the
    // member-list check (a->members == b->members, both NULL before
    // completion) both fail to recognize two such variants as the same
    // tag -- e.g. a forward-declared struct used as a function's return
    // type in both its prototype and its definition wrongly diagnosed as
    // "conflicting types". Set to the canonical (first-created, tag-
    // registered) Type's own address and preserved verbatim across the
    // `*ret = *ty` copies qualify_struct_type() makes for each variant;
    // plain (non-struct/union) types leave this NULL.
    Type *struct_id;
    bool is_signed_char; // signed char vs plain char (both have is_unsigned=false)
    bool is_vector; // GCC __attribute__((vector_size(N))): TY_STRUCT of N scalar
    // element-members, base = element type, align = total size
    bool is_transparent_union; // GCC __attribute__((__transparent_union__)):
    // TY_UNION whose members are all-mutually-assignment-compatible pointer
    // types (or all the same size); a function argument matching any one
    // member's type is passed exactly as that member, no boxing/copy needed.
    unsigned char qual; // TypeQual flags: const/volatile/restrict
    // Qualified INCOMPLETE struct/union variants (see qualify_struct_type
    // in parser.c): a `const struct S*` parsed while S was still
    // forward-declared must eventually read the finished type's members/
    // size/alignment, so it is registered here (head lives on the
    // canonical tag type; on a variant this field is the next link) and
    // struct_or_union_specifier() completes it in lockstep with the tag.
    // Without the variant, stamping the qualifier on the canonical type
    // would leak const onto every later declaration of the same tag.
    Type *qual_variants;
    // The qualifiers this variant's declaration requested, EXCLUDING the
    // canonical type's own quals; re-applied on top of the canonical's
    // quals when the completion sync overwrites the variant (see
    // struct_or_union_specifier).
    unsigned char use_qual;
    Type *base; // for pointer/array
    Member *members; // for struct
    bool has_body; // struct/union: a `{ ... }` body was parsed (distinct
    // from `members != NULL`: a GNU empty struct `{}` or a struct whose
    // only member is an anonymous zero-width bitfield `int : 0;` has no
    // Member nodes at all, `size == 0`, yet is a genuinely COMPLETE type,
    // not a forward declaration -- `size == 0 && !members` alone can't
    // tell those apart.
    Type *return_ty; // for function
    Type *param_types; // linked list of parameter types (for function)
    Contract *preconds; // pre(...) contract specifiers (for function)
    Contract *postconds; // post(...) contract specifiers (for function)
    Type *param_next; // next in parameter type list
    bool is_variadic; // for function
    bool is_oldstyle; // old-style (K&R) function definition / non-prototype ABI
    bool is_void_params; // explicit `(void)` parameter list: a real
    // zero-parameter prototype, distinct from an old-style/K&R empty `()`
    // (unspecified parameters, compatible with any definition). Both leave
    // param_types NULL, so this flag is what lets a redeclaration check
    // tell "genuinely no parameters" apart from "not yet specified".
    bool is_reproducible; // C23 [[reproducible]] function type attribute
    bool is_unsequenced; // C23 [[unsequenced]] function type attribute
    int pack_align; // #pragma pack(n) alignment, 0 = default
    unsigned char bitfield_mode;
    char *cleanup_func; // __attribute__((__cleanup__(func))) on the type
    char *name; // for parameter types: the parameter name
    Node *vla_len_expr; // VLA dimension expression (NULL = constant)
    void *vla_len_val; // LVar temporary for evaluated VLA dimension
    int64_t array_len; // array size (for both TY_ARRAY and TY_VLA constant fallback)
    int bitint_width; // N in _BitInt(N) (for TY_BITINT only)
};

static inline bool ty_const(const Type *t) { return t->qual & QUAL_CONST; }
static inline bool ty_volatile(const Type *t) { return t->qual & QUAL_VOLATILE; }
static inline bool ty_restrict(const Type *t) { return t->qual & QUAL_RESTRICT; }
static inline bool ty_atomic(const Type *t) { return t->qual & QUAL_ATOMIC; }

typedef struct Typedef Typedef;
struct Typedef {
    Typedef *next;
    Typedef *hash_next;
    char *name;
    Type *ty;
};

extern Type *ty_void;
extern Type *ty_bool;
extern Type *ty_int;
extern Type *ty_uint;
extern Type *ty_char;
extern Type *ty_uchar;
extern Type *ty_short;
extern Type *ty_ushort;
extern Type *ty_long;
extern Type *ty_ulong;
extern Type *ty_llong;
extern Type *ty_ullong;
extern Type *ty_int128;
extern Type *ty_uint128;
extern Type *ty_float;
extern Type *ty_double;
extern Type *ty_ldouble;
extern Type *ty_decimal32;
extern Type *ty_decimal64;
extern Type *ty_decimal128;
extern Type *ty_nullptr_t;
Type *size_t_type(void);

extern bool opt_O0;
extern bool opt_O1;
// -O3-only: NOT a real extra codegen optimization tier (rcc's own passes
// don't distinguish -O2/-O3 otherwise) -- gates the contract range
// prover (parser.c) alone, so it never runs at the default fast-compile
// levels. See parser.c's "Contract range prover" section.
extern bool opt_O3;
extern bool opt_finline;
extern bool opt_funroll;
extern bool opt_v;
/* Value for the __STDC_VERSION__ predefined macro, selected by -std=.
 * NULL means do not define it (C90/C89, which has no __STDC_VERSION__). */
extern const char *opt_std_version;
extern bool opt_gnu_mode;
extern bool opt_strict_ansi;
extern const char *opt_exec_charset;
extern bool opt_W;
extern bool opt_Werror;
// Set only by the literal "-Werror" flag, deliberately distinct from
// opt_Werror (which -pedantic-errors also sets, for promoting pedantic
// diagnostics to errors -- see main.c's own detailed comment at the
// -Werror parse site). Genuine compiler diagnostics that real GCC only
// promotes under an explicit bare -Werror (never under -pedantic-errors
// alone, confirmed directly against gcc) -- currently #warning -- must
// gate on this instead of opt_Werror.
extern bool opt_werror_flag;
extern bool opt_pedantic;
extern bool opt_Werror_unknown;
extern bool opt_Wno_homoglyph;
extern bool opt_Wno_c23_c2y_compat;
// -Wno-contract-assume-false: suppress the (on-by-default) warning when
// a contract_assume() is proven never-satisfiable -- by eval_const_expr
// (a literal condition) or, at -O3, the range prover -- and therefore
// compiles to an unconditional __builtin_unreachable() whose following
// code is dead-code-eliminated (see codegen.c's bi_unreachable handling).
extern bool opt_Wno_contract_assume_false;
extern bool opt_ms_bitfields;
extern bool opt_dM;
extern bool opt_E;
extern bool opt_g;
extern bool opt_pie;
extern bool opt_pic;
extern bool opt_time;
// -fdefer-ts (WG14 N3199 / TS 25755 experimental `defer` statement,
// matching clang's own flag name): gates `defer` keyword recognition
// in stmt() so ordinary code using `defer` as an identifier elsewhere
// in the corpus is unaffected when the flag isn't passed.
extern bool opt_defer_ts;
// codeql[cpp/commented-out-code]: trailing note names the real pragma this flag mirrors, not dead code
extern bool fenv_access; // #pragma STDC FENV_ACCESS state
extern int pack_align;
extern bool sse42_available;

bool is_integer(Type *ty);
bool is_flonum(Type *ty);
bool is_decimal(Type *ty);
bool is_complex(Type *ty);
bool is_number(Type *ty);
bool is_null_value_or_nullptr(Node *n);
Type *get_integer_type(int size, bool is_unsigned);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int64_t len);
Type *complex_type(Type *base);
Type *bitint_type(int width, bool is_unsigned);
// Return a copy of `ty` with `quals` TypeQual bits added, never
// mutating a shared struct/union Type object (parser.c's qualify_struct_type()
// handles the incomplete-struct qual-variant linking; see its comment).
Type *qualify_type_copy(Type *ty, unsigned char quals);
Type *decay_to_ptr(Type *arr_ty);
extern bool parser_used_wide_bitint;
extern bool parser_used_decimal; // _Decimal32/64/128 type or literal seen

typedef struct Reloc Reloc;
struct Reloc {
    Reloc *next;
    int offset;
    char *label;
    int addend;
    // Label-address DIFFERENCE (GCC's `&&label_a - &&label_b` computed-goto
    // jump-table idiom): when set, this Reloc means "the value at `offset`
    // is offset(label) - offset(label2), patched as `size` raw bytes" —
    // resolved by codegen.c as a same-object byte patch once both labels'
    // .text offsets are known (deferred past all function-body codegen;
    // there is no ELF/Mach-O relocation kind for a symbol difference).
    // `label`/`addend` are unused when label2 is set.
    char *label2;
    int size; // patch width in bytes (1/2/4/8) — only meaningful when label2 is set
};

typedef struct DiagEntry DiagEntry;
struct DiagEntry {
    DiagEntry *next;
    char *msg;
    bool is_error;
};


typedef struct LVar LVar;
struct LVar {
    LVar *next;
    LVar *hash_next;
    LVar *param_next;
    char *name;
    char *asm_name; // Assembly-level name (for static locals)
    char *alias_target; // __attribute__((alias("target")))
    char *section_name; // __attribute__((section("name")))
    int offset;
    Type *ty;
    bool is_local;
    bool is_extern;
    bool is_function;
    bool is_nested_fn;
    bool is_static; // static local variable
    bool is_inline;
    bool is_weak;
    bool is_used; // __attribute__((used)) — see Function.is_used
    bool has_init;
    bool has_visibility; // __attribute__((visibility("..."))) seen
    uint8_t visibility; // STV_* (obj.h) when has_visibility
    // True when append_reloc() (parser.c) records a relocation whose
    // target names this LVar and this LVar is_function: some global
    // variable's initializer takes this function's address (`&fn`,
    // possibly through a chain of const-index/member accesses).
    // Consulted only for a plain (non-static, non-extern, no forcing
    // redeclaration) `inline` function's SB_LOCAL-vs-SB_WEAK choice in
    // codegen.c: address-taking needs a linker-collapsible symbol so
    // `&fn` compares equal across every TU that takes it (see that
    // comment); a function only ever *called*, never address-taken,
    // keeps the narrower SB_LOCAL binding real GCC/Clang effectively
    // give it too (own copy, no cross-TU visibility needed).
    bool addr_taken;
    bool is_constexpr; // C23 constexpr object
    int64_t init_val;
    char *init_data;
    int init_size;
    Reloc *relocs;
    char *cleanup_func; // __attribute__((__cleanup__(func)))
    // C23 `defer` (WG14 N3199 / `-fdefer-ts`): a raw, already-parsed
    // statement to run at scope exit, in the same LIFO position as an
    // ordinary cleanup_func entry (this LVar is a zero-storage marker
    // synthesized directly onto the `locals` chain at the `defer`
    // statement's own point, not a real declaration -- see parser.c's
    // `stmt()`). Mutually exclusive with cleanup_func.
    Node *defer_stmt;
    bool is_tls; // __thread / _Thread_local
    bool is_register; // register compound literal, or a GCC global register variable
    bool is_global_reg; // `register TYPE name asm("regname");` at file scope (GCC
    // "Global Register Variables" extension, e.g. x86's
    // `register unsigned long current_stack_pointer asm("rsp");`): the
    // identifier names a hardware register directly, never memory — no
    // symbol/storage is emitted, and every read materializes straight
    // from `global_reg_name`'s physical register (see codegen.c's
    // ND_LVAR handling). asm_name is left NULL for these (it's not a
    // linker symbol rename); the decoded asm(...) string lives here
    // instead, kept separate so ordinary asm-renamed globals are
    // unaffected.
    char *global_reg_name; // e.g. "rsp", "sp" — valid iff is_global_reg
    bool is_deprecated; // C23 [[deprecated]]
    char *deprecated_msg; // C23 [[deprecated("reason")]]
    bool is_nodiscard; // C23 [[nodiscard]]
    char *nodiscard_msg; // C23 [[nodiscard("reason")]]
    bool is_constructor; // __attribute__((constructor)) seen on a decl
    bool is_destructor; // __attribute__((destructor)) seen on a decl
    bool is_reproducible; // C23 [[reproducible]]
    bool is_unsequenced; // C23 [[unsequenced]]
    char *diag_warning; // __attribute__((warning("msg")))
    char *diag_error; // __attribute__((error("msg")))
    DiagEntry *diag_entries; // __attribute__((diagnose_if(...)))
    // Name of the function this global-storage-duration variable was
    // *declared inside* (a block-scope `static` local — e.g. a compound
    // literal's or a `static int counter;`'s backing storage), or NULL
    // for a true file-scope global. See eliminate_unused_static_inline()
    // in opt.c: a block-scope static that lexically lives inside a
    // function whose body never gets emitted must not be emitted either
    // — its initializer's relocations (e.g. a `DEFINE_STATIC_CALL`-style
    // addressable-marker local pointing at a static-call key) are real
    // undefined-symbol references otherwise, even though the enclosing
    // function was correctly recognized as dead.
    char *decl_fn_name;
    // True for the compiler-injected ARM64/Apple synthetic prelude
    // declarations (parser.c's parse(): a handful of well-known libc
    // functions predeclared with the correct return type ahead of the
    // real source, working around an ABI issue). User code re-declaring
    // one of these more specifically (e.g. `extern int memcmp(const
    // char *, const char *, size_t);` vs the prelude's `const void *`)
    // is a long-tolerated, harmless pattern for well-known standard
    // functions, not a real conflict — the prototype-redeclaration
    // check must not compare against these.
    bool is_synthetic_prelude;
};

// GNU nested functions: fixed frame offset (from rbp/x29) where every
// nested Function spills its incoming static-chain pointer at prologue
// entry, reserved on top of the ordinary stack_offset=80 base (see
// parser.c's nested-function-definition path and codegen.c's prologue).
// A single well-known offset, uniform across every nested function's own
// frame, is what lets chain-walking (Node.chain_depth indirections)
// dereference through an arbitrary number of ancestor frames without
// needing per-function layout metadata at the point of use.
#define CHAIN_SLOT_OFFSET 88
// Nonlocal goto (a nested function's `goto` targeting a label declared via
// __label__ in an ancestor function) must restore that ancestor's rsp/sp
// exactly, not just its rbp/x29: every function's epilogue is `add
// sp,#frame_size; pop/ldp; ret`, and frame_size is a per-function,
// codegen-time value the nonlocal-goto site (compiled independently,
// often *before* the target ancestor - see parser.c's
// nested-function-definition path) cannot know, nor can it rely on a
// caller-propagated live sp (call-site-local register/argument staging
// transiently perturbs it around the very call that would capture it).
// So a goto *target* function - one whose __label__ is actually reached
// by a nested descendant (see is_goto_target_fn below) - self-records
// its own just-established, stable sp here, once, at its own prologue;
// the goto site chain-walks to the target's rbp/x29 (CHAIN_SLOT_OFFSET)
// and reads this slot back directly.
#define CHAIN_RSP_OFFSET 96

// GNU nested function used as a *value* (not a direct call) - passed as
// a function pointer, stored, returned - needs a runtime trampoline: a
// small per-activation stub, allocated in the enclosing function's own
// frame, that loads the static-chain pointer and jumps to the nested
// function's real code (see codegen.c's ND_LVAR function-value branch).
// 32 bytes covers the largest template (ARM64: 4 fixed instruction
// words + 8-byte target address + 8-byte chain value); x86-64 only
// needs 22 but shares the same size/alignment for a uniform allocator.
#define TRAMPOLINE_SIZE 32

void check_type(Node *node);
// Build a GCC __attribute__((vector_size(N))) type (TY_STRUCT + is_vector
// + per-lane members). Shared by parser.c's declarator path (the
// headers' __m128/__m128i typedefs) and type.c's __builtin_ia32_*
// return-type classifier, so both see the exact same Type construction.
Type *rcc_make_vector_type(Type *elem, int total_size);
LVar *find_global_name(char *name);
// True if `name` (an interned function-name pointer) is the target of an
// actual nonlocal goto from a nested descendant - see parser.c. Valid to
// call only after parse() has fully returned (codegen() runs as a
// separate, later pass - see lib.c/main.c).
bool is_goto_target_fn(const char *name);

#define MAX_ASM_OPERANDS 30

typedef struct AsmOperand AsmOperand;
struct AsmOperand {
    char constraint[16]; // e.g. "=m", "r", "=r"
    char name[32]; // named operand [name] (empty if unnamed)
    char asm_str[64]; // computed AT&T operand string (filled by codegen)
    Node *expr; // C expression for the operand
    int reg; // allocated reg index (-1 if unused)
    bool is_memory; // 'm' in constraint
    bool is_output; // '=' or '+' in constraint
    bool is_rw; // '+' (read-write) in constraint
};

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_MOD, // %
    ND_SHL, // <<
    ND_SHR, // >>
    ND_BITAND, // &
    ND_BITXOR, // ^
    ND_BITOR, // |
    ND_EQ, // ==
    ND_NE, // !=
    ND_LT, // <
    ND_LE, // <=
    ND_ASSIGN, // =
    ND_POST_INC, // postfix ++
    ND_POST_DEC, // postfix --
    ND_PRE_INC, // prefix ++
    ND_PRE_DEC, // prefix --
    ND_ADDR, // &
    ND_DEREF, // *
    ND_CAST, // cast
    ND_BITNOT, // ~
    ND_FUNCALL, // Function call
    ND_LVAR, // Local variable
    ND_NUM, // Integer
    ND_RETURN, // "return"
    ND_IF, // "if"
    ND_FOR, // "for" or "while"
    ND_DO, // "do"
    ND_SWITCH, // "switch"
    ND_CASE, // "case" or "default"
    ND_BREAK, // "break"
    ND_CONTINUE, // "continue"
    ND_GOTO, // "goto"
    ND_GOTO_IND, // "goto *expr" (computed goto)
    ND_LABEL, // label:
    ND_LABEL_VAL, // &&label (label address)
    ND_STMT_EXPR, // GNU statement expression
    ND_BLOCK, // { ... }
    ND_EXPR_STMT, // Expression statement
    ND_NULL, // Empty statement
    ND_STR, // String literal
    ND_MEMBER, // Struct member access
    ND_LOGAND, // &&
    ND_LOGOR, // ||
    ND_COND, // ?:
    ND_COMMA, // ,
    ND_SIZEOF, // sizeof
    ND_FNUM, // Float literal
    ND_REAL, // __real__ (extract real part of complex)
    ND_IMAG, // __imag__ (extract imag part of complex)
    ND_NEG, // Unary minus
    ND_NOT, // Logical not
    ND_ZERO_INIT, // Zero-fill a local variable (lhs=ND_LVAR)
    ND_ASM, // inline asm statement
    ND_VA_START, // "va_start"
    ND_VA_COPY, // "va_copy"
    ND_VA_ARG, // "va_arg"
    ND_VA_ARG_PACK, // "__builtin_va_arg_pack" placeholder
    ND_VA_ARG_PACK_LEN, // "__builtin_va_arg_pack_len" placeholder
    ND_ALLOCA, // VLA stack allocation
    ND_ALLOCA_ZINIT, // VLA stack allocation + zero init
    ND_CHAIN, // Chain expressions (evaluate lhs, result is rhs)
    ND_ATOMIC_LOAD, // __atomic_load
    ND_ATOMIC_STORE, // __atomic_store (stores via expr statement)
    ND_ATOMIC_EXCHANGE, // __atomic_exchange
    ND_ATOMIC_CAS, // __atomic_compare_exchange
    ND_ATOMIC_FENCE, // __atomic_thread_fence / __atomic_signal_fence
    ND_ATOMIC_FETCH_OP, // __atomic_fetch_add/sub/or/xor/and/nand
} NodeKind;

struct Node {
    NodeKind kind; // Node kind
    Node *next; // Next node (for blocks or statements)

    Token *tok; // Representative token for this node
    Type *ty; // AST node type

    Node *lhs; // Left-hand side
    Node *rhs; // Right-hand side

    // "if" or "for" statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    // Block or arguments
    Node *body;
    Node *args; // Linked list of args
    Node *stmt_expr_result;

    // Function call
    char *funcname;
    char *label_name;

    // String literal
    char *str;
    int str_id;

    // Local variable
    LVar *var;
    // Nested-function static-chain access: 0 = var belongs to the function
    // currently being codegen'd (ordinary rbp/x29-relative access); N>0 =
    // var was resolved N enclosing-function levels up (see parser.c's
    // FnCtx stack) and must be reached via N indirections through each
    // ancestor's saved static-chain pointer (CHAIN_SLOT_OFFSET) before
    // applying var->offset in that ancestor's own frame.
    int chain_depth;

    // Cleanup range for control-flow that exits scopes
    LVar *cleanup_begin;
    LVar *cleanup_end;
    LVar *continue_cleanup_end;
    // ND_RETURN only: a 16-byte scratch stack slot (lazily allocated by
    // parser.c when this return has a non-empty cleanup_begin..
    // cleanup_end range) codegen spills the return value's ABI
    // register(s) into before running pending cleanup/defer code and
    // reloads from afterward -- a defer body or __attribute__((cleanup))
    // function is arbitrary code that can itself clobber the return
    // register before the final jump to the epilogue. NULL when there
    // is nothing pending on this return path (the common case).
    LVar *defer_retspill;
    int64_t val; // Used if kind == ND_NUM
    int64_t val2; // Used if kind == ND_NUM: high 64 bits (TY_DECIMAL128)
    double fval; // Used if kind == ND_FNUM
    int array_len; // Used if kind == TY_ARRAY

    // Struct member access
    Member *member;

    // switch/case
    Node *case_next;
    Node *default_case;
    int64_t case_val;
    int64_t case_end; // for case ranges (GNU extension)
    bool is_case_range;
    int label_id;

    // C2Y labeled break/continue: on a loop (ND_FOR/ND_DO) or switch
    // (ND_SWITCH) node, `label_name` holds the first label directly
    // preceding it (`MainLoop: for (...)`, `k: l: switch (...)`); further
    // labels chain via label_next (each a dummy node carrying label_name).
    // On an ND_BREAK/ND_CONTINUE with a label, `target_loop` is the node the
    // break/continue must exit/continue; `parent_loop` chains to the
    // lexically enclosing loop/switch (the parser's current_ctrl) so the
    // target can be found several nesting levels out. rcc previously
    // dropped the label: `continue MainLoop;` inside a nested loop was
    // emitted as a plain `continue` of the INNERMOST loop, skipping the
    // labeled loop's continuation semantics entirely (found via c23doku's
    // graph_color_c2y.c, whose backtracking solver diverged and then
    // smashed sort_buf on a 9x9 puzzle).
    Node *parent_loop;
    Node *target_loop;
    Node *label_next;

    // ND_ASM (inline asm statement)
    char *asm_template; // raw template string (with escape sequences decoded)
    int asm_nout; // number of output operands
    int asm_noperands; // outputs + inputs
    AsmOperand *asm_ops; // [0..noperands-1], outputs first
    char **asm_goto_labels; // goto label names
    int asm_ngoto;

    // Atomic operations
    int atomic_ord; // MemoryOrder
    int atomic_ord2; // second memory order (for CAS)
    int atomic_fetch_op; // 0=add, 1=sub, 2=or, 3=xor, 4=and, 5=nand
    bool atomic_weak; // weak flag for CAS
    bool atomic_is_store; // true for __atomic_store (stores val, no return)
    bool atomic_signal_fence; // true for __atomic_signal_fence
    // true for __sync_val_compare_and_swap: return the ORIGINAL *ptr
    // value (always, success or failure), not the success/fail bool
    // __sync_bool_compare_and_swap / __atomic_compare_exchange return.
    bool atomic_cas_return_old;
    // Set once eval_const_expr_impl() has warned about signed integer
    // overflow folding this node (see warn_const_int_overflow in
    // parser.c) -- the same node can be re-evaluated from several call
    // sites (speculative array-size/alignment attempts, etc.); without
    // this the identical diagnostic would repeat once per evaluation.
    bool overflow_warned;
};

typedef struct Function Function;
struct Function {
    Function *next;
    // True for a GNU nested function (function definition syntactically
    // inside another function's body); false for an ordinary top-level
    // function. Nested functions are otherwise ordinary top-level
    // Functions (own TLItem, own prologue/epilogue, own mangled
    // asm_name) — this flag only marks the static-chain relationship
    // codegen needs: at prologue entry, a nested function spills its
    // incoming static-chain pointer (physical %r10) to CHAIN_SLOT_OFFSET
    // in its own frame, so nested-body chain_depth>0 accesses (see
    // Node.chain_depth) and any doubly-nested descendant can walk it.
    bool is_nested;
    char *name;
    char *asm_name;
    char *alias_target;
    Type *ty;
    LVar *params;
    LVar *locals;
    Node *body;
    int stack_size;
    bool is_variadic;
    bool is_constructor;
    bool is_destructor;
    bool is_inline;
    bool is_gnu_inline; // GNU89 semantics: extern inline is never emitted
    bool is_always_inline; // __attribute__((always_inline)): force inline at -O0 too
    bool is_static;
    bool is_extern;
    bool is_weak;
    bool is_used; // __attribute__((used)): exempts from
    bool has_visibility; // __attribute__((visibility("..."))) seen
    uint8_t visibility; // STV_* (obj.h) when has_visibility
    // eliminate_unused_static_inline()'s omission of a never-called
    // `static inline` function, matching real GCC (verified: without
    // `used`, GCC omits an uncalled `static inline` function's body at
    // every -O level, even -O0; with it, GCC always emits one).
    bool dce_live; // scratch flag for eliminate_unused_static_inline()'s
    // reachability pass; meaningless outside that pass
    bool has_def;
    bool dealloc_vla; // restore RSP from VLA base on scope exit
    // FMV (Function Multi-Versioning): __attribute__((target_clones(...)))
    char **target_clones; // NULL-terminated array of clone target strings
    int n_target_clones; // count of target_clones entries (excluding terminator)
    char *target_attr; // __attribute__((target("..."))) string
};

typedef struct StrLit StrLit;
struct StrLit {
    StrLit *next;
    char *str;
    int id;
    int prefix; // 0 = regular, 'L' = wide, 'u' = char16_t, 'U' = char32_t
    int elem_size; // size of each character element (1 for regular, 2 or 4 for wide)
    int len; // actual byte length of string content (includes embedded NULs)
    int wchar_count; // number of Unicode characters (for wide strings)
};

typedef struct TLItem TLItem;
struct TLItem {
    enum { TL_FUNC,
           TL_ASM } kind;
    Function *fn; // valid if kind == TL_FUNC
    char *asm_str; // valid if kind == TL_ASM
    TLItem *next;
    TLItem *hash_next;
};

typedef struct Program Program;
struct Program {
    TLItem *items;
    LVar *globals;
    StrLit *strs;
    char *in_path;
};

// Parser entry point
Program *parse(Token *tok);

//
// CodeGen
//
struct ObjFile;
bool va_arg_need_copy(Type *ty);
struct ObjFile *codegen(Program *prog);
void objfile_free(struct ObjFile *obj);
int elf_write(struct ObjFile *obj, const char *path);
int macho_write(struct ObjFile *obj, const char *path);
int coff_write(struct ObjFile *obj, const char *path);
int objfile_add_debug_file(struct ObjFile *obj, char *filename);
void objfile_add_debug_line(struct ObjFile *obj, uint64_t text_offset, int file_idx, int line);
void objfile_flush_debug_line(struct ObjFile *obj, uint64_t text_end);
bool objfile_has_debug(struct ObjFile *obj);

// VLA
Type *vla_of(Type *base, Node *expr, int64_t arr_len);

// Optimizer (CTFE)
void optimize(Program *prog);
// Run when optimize() itself won't (no -O1/-finline/-funroll): expands
// ONLY __always_inline__ calls, matching real GCC's -O0 behavior for
// `extern inline ... __attribute__((always_inline))` header wrappers.
void always_inline_pass(Program *prog);
// Drop TL_FUNC entries for `static inline` functions that nothing in this
// translation unit calls or takes the address of (see opt.c for the full
// rationale) — matches real GCC/Clang, which never emit such a function's
// body at any optimization level. Called unconditionally (not gated on
// -O1+): this is standard-permitted inline-definition omission, not an
// optimization.
void eliminate_unused_static_inline(Program *prog);
bool eval_const_expr(Node *node, long long *val);
// Set (and restored) around a speculative eval_const_expr() probe whose
// caller only cares whether/what the value is, not whether folding it
// hit UB -- e.g. is_null_pointer_constant() in type.c, invoked on both
// arms of every conditional expression regardless of which arm a
// constant condition selects. See warn_const_int_overflow in parser.c.
extern bool suppress_const_overflow_warn;

// Unicode identifiers
uint32_t decode_utf8(char **new_pos, char *p);
bool is32_ident1(uint32_t c);
bool is32_ident2(uint32_t c);
int utf8_len(char *str);
// Check identifier for Unicode script-mixing homoglyph issues (TR39).
// The SSE2/NEON fast-path reads up to 16 bytes from `name`, potentially
// past `len` onto the next arena allocation — safe because arena_alloc()
// rounds every allocation up to 16 bytes and calloc() zeroes the padding.
const char *u8ident_check_ident_align16(const char *name, int len);
void u8ident_allow_script(const char *name);


#endif // RCC_H
