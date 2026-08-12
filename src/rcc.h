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
    // C99 6.10.3.4p2 "blue paint": the macro whose expansion produced this
    // token. That macro must never be re-expanded from this token, even
    // after the expansion frame that painted it has been popped and the
    // token is rescanned as part of an outer macro's argument or
    // replacement list (e.g. glibc's `#define si_uid _sifields._kill.si_uid`
    // used inside nested macros). First paint wins: a token keeps the paint
    // of the innermost macro that produced it.
    struct Macro *blue;
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
// -nostdinc: skip system include paths
extern bool opt_nostdinc;
// Make dependency generation (-Wp,-MMD, / -MD / -MMD / -MF / -MT / -MQ / -MP)
extern const char *opt_depfile;
extern const char *opt_dep_target;
extern bool opt_gen_deps;
extern bool opt_dep_phony;
// -fmacro-prefix-map=old=new
extern const char *opt_prefix_map_old;
extern const char *opt_prefix_map_new;
// -include <file>: pre-include before main source
void add_preinclude(const char *path);
void add_prefix_map(const char *old, const char *new_str);
// Write dependency file after preprocessing
void write_dep_file(const char *out_path, const char *main_fpath);
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
extern char *bi_mul_overflow, *bi_mul_overflow_p;
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
extern char *bi_s_alloca;
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
} TypeKind;

typedef struct Node Node;
typedef struct Type Type;
typedef struct Member Member;


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
    bool is_signed_char; // signed char vs plain char (both have is_unsigned=false)
    bool is_vector; // GCC __attribute__((vector_size(N))): TY_STRUCT of N scalar
    // element-members, base = element type, align = total size
    bool is_transparent_union; // GCC __attribute__((__transparent_union__)):
    // TY_UNION whose members are all-mutually-assignment-compatible pointer
    // types (or all the same size); a function argument matching any one
    // member's type is passed exactly as that member, no boxing/copy needed.
    unsigned char qual; // TypeQual flags: const/volatile/restrict
    Type *base; // for pointer/array
    Member *members; // for struct
    Type *return_ty; // for function
    Type *param_types; // linked list of parameter types (for function)
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
extern Type *ty_nullptr_t;

extern bool opt_O0;
extern bool opt_O1;
extern bool opt_finline;
extern bool opt_funroll;
extern bool opt_v;
/* Value for the __STDC_VERSION__ predefined macro, selected by -std=.
 * NULL means do not define it (C90/C89, which has no __STDC_VERSION__). */
extern const char *opt_std_version;
extern bool opt_gnu_mode;
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
extern bool opt_ms_bitfields;
extern bool opt_dM;
extern bool opt_g;
extern bool opt_pie;
extern bool opt_pic;
extern bool opt_time;
// codeql[cpp/commented-out-code]: trailing note names the real pragma this flag mirrors, not dead code
extern bool fenv_access; // #pragma STDC FENV_ACCESS state
extern int pack_align;
extern bool sse42_available;

bool is_integer(Type *ty);
bool is_flonum(Type *ty);
bool is_complex(Type *ty);
bool is_number(Type *ty);
Type *get_integer_type(int size, bool is_unsigned);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int64_t len);
Type *complex_type(Type *base);
Type *bitint_type(int width, bool is_unsigned);

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
    bool is_constexpr; // C23 constexpr object
    int64_t init_val;
    char *init_data;
    int init_size;
    Reloc *relocs;
    char *cleanup_func; // __attribute__((__cleanup__(func)))
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

    int64_t val; // Used if kind == ND_NUM
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
    bool is_static;
    bool is_extern;
    bool is_weak;
    bool is_used; // __attribute__((used)): exempts from
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
// Drop TL_FUNC entries for `static inline` functions that nothing in this
// translation unit calls or takes the address of (see opt.c for the full
// rationale) — matches real GCC/Clang, which never emit such a function's
// body at any optimization level. Called unconditionally (not gated on
// -O1+): this is standard-permitted inline-definition omission, not an
// optimization.
void eliminate_unused_static_inline(Program *prog);
bool eval_const_expr(Node *node, long long *val);

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
