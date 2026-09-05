// SPDX-License-Identifier: LGPL-2.1-or-later
#include <ctype.h>
// Derived from chibicc by Rui Ueyama.
#include "rcc.h"
#include "obj.h" // STV_* visibility constants

// Decimal-literal folding: these come from the bundled libdfp.a (libbid
// core, LGPL-2.1, see lib/libdfp/), which rcc itself links so it can fold
// _Decimal32/64/128 literals into IEEE 754-2008 BID bits at compile time.
// The generated code calls the same __bid_* symbols at run time.
// musl lacks libdfp; decimal float support is disabled.
#ifndef __MUSL__
typedef struct BID_UINT128 {
    unsigned long long w[2];
} BID_UINT128;
extern unsigned long long __bid64_from_string(char *);
extern BID_UINT128 __bid128_from_string(char *);
extern unsigned long long __bid64_to_bid32(unsigned long long);
extern unsigned long long __bid64_from_int64(long long);
extern BID_UINT128 __bid64_to_bid128(unsigned long long);
#endif

typedef struct VarAttr VarAttr;
typedef struct TagScope TagScope;
typedef struct EnumConst EnumConst;

struct VarAttr {
    bool is_typedef;
    bool is_extern;
    bool is_static;
    bool is_inline;
    bool is_gnu_inline;
    bool is_always_inline;
    bool is_weak;
    bool is_used; // __attribute__((used)) / __attribute__((__used__))
    bool is_tls;
    bool has_visibility; // __attribute__((visibility("..."))) seen
    uint8_t visibility; // STV_* when has_visibility
    bool has_type;
    bool is_packed;
    bool is_constexpr;
    bool is_auto_type;
    bool is_auto;
    bool is_register;
    char *diag_warning;
    char *diag_error;
    DiagEntry *diag_entries;
    unsigned char bitfield_mode;
    bool is_noreturn;
    bool is_noreturn_std;
    bool has_alignas;
    bool is_deprecated;
    char *deprecated_msg;
    bool is_reproducible;
    bool is_unsequenced;
    bool is_transparent_union;
};

struct TagScope {
    TagScope *next;
    TagScope *hash_next;
    char *name;
    Type *ty;
    int depth; // block depth of the declaration (same-scope redef detection)
};

struct EnumConst {
    EnumConst *next;
    EnumConst *hash_next;
    char *name;
    int64_t val;
    Type *ty; // C23 enumerator type (NULL = int)
};

typedef struct EnumTag EnumTag;
struct EnumTag {
    EnumTag *next;
    char *name;
    Type *ty;
    int depth; // block depth of the declaration (same-scope redef detection)
    bool members_int; // completed enumerators have type int (all fit in int)
};
static EnumTag *enum_tags;

#define kw_is(tok, flag) ((tok)->kw != ID_NONE && (kw_flags[(tok)->kw] & (flag)))

static LVar *locals;
static LVar *globals;

// Functions defined as "extern inline ... (...) { ... __builtin_va_arg_pack() ... }"
// are never emitted; instead each call site is inlined as a statement
// expression with __builtin_va_arg_pack() replaced by that call's trailing
// variadic arguments. See node_uses_va_arg_pack/inline_pack_call below.
typedef struct InlinePackFn InlinePackFn;
struct InlinePackFn {
    InlinePackFn *next;
    char *name;
    Function *fn;
};
static InlinePackFn *inline_pack_fns;
static int inline_pack_counter;

#define GLOBAL_HASH_SIZE 8192
static LVar *global_htab[GLOBAL_HASH_SIZE];

// Hash an identifier name by its pointer, not its bytes.
//
// Identifier names come from str_intern() (see tokenize()), and every table
// keyed by this hash -- global_htab, typedef_htab, tag_htab, enum_htab --
// resolves a bucket with `x->name == name`, pointer identity, not strcmp.
// Keying on the pointer therefore cannot change a lookup outcome: a byte-equal
// but distinct pointer failed that comparison before and now merely hashes to
// a different bucket, still not found. Walking the string re-hashed bytes
// str_intern() had already hashed, once per identifier occurrence.
//
// Callers doing `% GLOBAL_HASH_SIZE` / `% SCOPE_HASH_SIZE` keep working: both
// are compile-time powers of two, so the modulo is already an AND, and the
// low bits returned here are well mixed.
static uint32_t hash_name(const char *s) {
    uint64_t v = (uint64_t)(uintptr_t)s;
    v *= 0x9E3779B97F4A7C15ull; // fibonacci mix; interned ptrs are 8-byte aligned
    return (uint32_t)(v >> 32);
}

static void global_htab_add(LVar *var) {
    uint32_t h = hash_name(var->name) % GLOBAL_HASH_SIZE;
    var->hash_next = global_htab[h];
    global_htab[h] = var;
}

#define SCOPE_HASH_SIZE 4096

static Typedef *typedefs;
static Typedef *typedef_htab[SCOPE_HASH_SIZE];

typedef struct TypedefLog TypedefLog;
struct TypedefLog {
    uint32_t h;
    Typedef *prev;
    TypedefLog *next;
};
static TypedefLog *typedef_log;

static TagScope *tags;
static TagScope *tag_htab[SCOPE_HASH_SIZE];

typedef struct TagLog TagLog;
struct TagLog {
    uint32_t h;
    TagScope *prev;
    TagLog *next;
};
static TagLog *tag_log;

static EnumConst *enum_consts;
static EnumConst *enum_htab[SCOPE_HASH_SIZE];

typedef struct EnumLog EnumLog;
struct EnumLog {
    uint32_t h;
    EnumConst *prev;
    EnumLog *next;
};
static EnumLog *enum_log;

static TypedefLog *typedef_scope_checkpoint(void) { return typedef_log; }
static void typedef_scope_restore(TypedefLog *cp) {
    while (typedef_log != cp) {
        TypedefLog *log = typedef_log;
        typedef_log = log->next;
        typedef_htab[log->h] = log->prev;
    }
}

static TagLog *tag_scope_checkpoint(void) { return tag_log; }
static void tag_scope_restore(TagLog *cp) {
    while (tag_log != cp) {
        TagLog *log = tag_log;
        tag_log = log->next;
        tag_htab[log->h] = log->prev;
    }
}

static EnumLog *enum_scope_checkpoint(void) { return enum_log; }
static void enum_scope_restore(EnumLog *cp) {
    while (enum_log != cp) {
        EnumLog *log = enum_log;
        enum_log = log->next;
        enum_htab[log->h] = log->prev;
    }
}

static void typedef_htab_add(Typedef *td) {
    uint32_t h = hash_name(td->name) % SCOPE_HASH_SIZE;
    TypedefLog *log = arena_alloc(sizeof(TypedefLog));
    log->h = h;
    log->prev = typedef_htab[h];
    log->next = typedef_log;
    typedef_log = log;
    td->hash_next = typedef_htab[h];
    typedef_htab[h] = td;
}

static void tag_htab_add(TagScope *tag) {
    uint32_t h = hash_name(tag->name) % SCOPE_HASH_SIZE;
    TagLog *log = arena_alloc(sizeof(TagLog));
    log->h = h;
    log->prev = tag_htab[h];
    log->next = tag_log;
    tag_log = log;
    tag->hash_next = tag_htab[h];
    tag_htab[h] = tag;
}

static void enum_htab_add(EnumConst *ec) {
    uint32_t h = hash_name(ec->name) % SCOPE_HASH_SIZE;
    EnumLog *log = arena_alloc(sizeof(EnumLog));
    log->h = h;
    log->prev = enum_htab[h];
    log->next = enum_log;
    enum_log = log;
    ec->hash_next = enum_htab[h];
    enum_htab[h] = ec;
}

static int stack_offset;
static char *pending_cleanup_func;
// C23 `defer` (-fdefer-ts): true while parsing a defer statement's own
// substatement -- a `return` inside is ill-formed (WG14 N3199: a defer
// body executes during scope unwind and cannot itself return a value
// from the enclosing function).
static bool in_defer_body;
static Token *pending_cleanup_tok;
static bool pending_constructor;
static bool pending_destructor;
static int pending_mode; // 0=none, 1=QI, 2=HI, 3=SI, 4=DI, 5=TI
static int pending_vector_size; // GCC __attribute__((vector_size(N))): total bytes, 0=none
static char *pending_asm_name;
static char *pending_alias_target;
static char *pending_section_name;
static char *pending_target_attr; // __attribute__((target("...")))
static char **pending_target_clones; // __attribute__((target_clones(...)))
static int pending_target_clones_n;
// Set by declarator() when it consumes a trailing __attribute__((transparent_union))
// right after the identifier (declarator() is called with attr=NULL from most
// sites, so it can't write directly into the caller's VarAttr) — the
// top-level/typedef declaration loop reads and resets this right after
// calling declarator(). Mirrors pending_alias_target/pending_cleanup_func.
static bool pending_transparent_union;
// Same as pending_transparent_union, but for __attribute__((weak))/__weak__
// that appears either between the pointer star(s) and the declared name,
// or trailing right after the name itself (kprobe_opcode_t *
// __attribute__((__weak__)) fn(...); int x __attribute__((weak));).
// Set by declarator(), consumed by both the function-definition handler
// and the plain global-variable declaration path.
static bool pending_weak;
static bool pending_visibility_set; // trailing __attribute__((visibility(...)))
static uint8_t pending_visibility; // STV_* when pending_visibility_set
// VLA-containing struct: emit size-capture code before the next statement
static Node *pending_vla_struct_capture;

static StrLit *str_lits;
static int str_lit_counter;

static Node *current_switch;
static Node *current_loop;
// Innermost enclosing loop OR switch being parsed (chains through each
// node's parent_loop). C23/C2Y labeled `break label;` / `continue label;`
// resolution walks this chain so a target loop/switch several nesting
// levels out — possibly through intervening switches — is reachable.
static Node *current_ctrl;
// Set by `label: for|while|do|switch` just before the statement is parsed
// (C2Y labeled statements); the loop/switch parse consumes the names as
// its labels (first in label_name, extras chained via label_next), which
// labeled `continue label;` / `break label;` resolve against. Set BEFORE
// stmt() so the node carries the labels while its body is still parsed.
static char *pending_loop_labels[32];
static int pending_loop_labels_n;

static Node *new_node(NodeKind kind, Token *tok); // declared below; needed by consume_pending_loop_labels

// Attach the pending label chain (accumulated by `lbl1: lbl2: ...` directly
// before this statement) to a loop/switch node: the first name goes into
// label_name, the rest onto the label_next chain.
static void consume_pending_loop_labels(Node *node, Token *tok) {
    if (pending_loop_labels_n <= 0)
        return;
    node->label_name = pending_loop_labels[0];
    Node **tail = &node->label_next;
    for (int i = 1; i < pending_loop_labels_n; i++) {
        Node *ln = new_node(ND_NULL, tok);
        ln->label_name = pending_loop_labels[i];
        *tail = ln;
        tail = &ln->next;
    }
    pending_loop_labels_n = 0;
}

// Does `node` (loop or switch) carry the label `name` (first or chained)?
static bool node_has_label(Node *node, char *name) {
    if (!node || !name)
        return false;
    if (node->label_name && !strcmp(node->label_name, name))
        return true;
    for (Node *ln = node->label_next; ln; ln = ln->next)
        if (ln->label_name && !strcmp(ln->label_name, name))
            return true;
    return false;
}
static int static_local_counter;
static LVar *current_fn_scope_locals;
static char *parser_current_fn;
static bool current_fn_is_inline; // parsing body of a non-static inline fn
static int current_block_depth;
// True for the duration of global_initializer()'s call tree (a genuine
// static/global-duration object's own initializer, including nested
// compound literals reached through it) -- as opposed to merely being
// AT block depth 0 syntactically, which is ALSO true while parsing a
// function prototype's parameter-list array-size expression (C23
// 6.5.2.5p10 gives a compound literal there automatic, not static,
// storage duration -- torture/c23-complit-1.c's `void f(int
// a[(int){x}]);` regressed when file-scope detection used
// current_block_depth == 0 alone).
static bool in_global_var_init;
// Set for the duration of a BEST-EFFORT attempt to also compile-time-fold
// a local (non-static) compound literal's value for later constexpr member
// access (see the "all_const" prescan below global_initializer()'s call at
// its speculative call site). That prescan only shallow-scans TOP-LEVEL
// tokens and blindly skips over any parenthesized group without checking
// its contents, so it cannot tell a genuinely constant parenthesized
// subexpression from one hiding a function call or `&local_var` -- e.g.
// postgres's `list_make1(list_make1(&rte))` (pg_list.h's list_make1/
// list_make_ptr_cell macros), where `.ptr_value = (list_make1_impl(...))`
// looks "all constant" to the shallow scan (the whole call is swallowed as
// one opaque parenthesized group) but is a genuine runtime call. Since
// this fold is opportunistic, a failure deep in global_init_one() (a
// non-constant value on a var that in_global_var_init's OWN propagation
// mis-marked as file-scope, or a plain unfoldable expression) must be
// treated as "give up quietly", not a hard compile error.
static bool in_speculative_const_fold;
// Set alongside every error suppressed by in_speculative_const_fold, so
// the speculative call site can tell "the whole thing folded cleanly" from
// "some member silently failed to fold" -- without this, a partial fold
// (some members written correctly, one left as un-set arena bytes) would
// still get `var->has_init = true` (set unconditionally by the caller
// before the fold attempt) and then be marked `is_constexpr = true`,
// letting later constant-expression reads (e.g. inside
// `_Static_assert(__builtin_types_compatible_p(...))`) silently pick up
// garbage bytes for the unfolded member instead of correctly failing/using
// the real runtime value.
static bool speculative_fold_failed;
static bool suppress_fn_scope_update;
static bool fn_uses_vla;
// Set around parsing a `constexpr`-qualified object's or compound
// literal's initializer. GCC's "braces around scalar initializer"
// warning (real GCC, unconditionally) fires for excess brace-nesting
// around a struct/union member's own initializer; tinycc's own
// tests2/90_struct-init.c relies on tinycc NOT diagnosing the identical
// plain (non-constexpr) pattern (its committed .expect has no warning
// line), while gcc torture's c23-constexpr-1.c dg-warns for the exact
// same shape but only ever inside `constexpr` declarations/compound
// literals. Gating on this flag matches both reference suites instead
// of picking one at the other's expense.
static bool in_constexpr_init;

typedef struct LabelScope LabelScope;
typedef struct PendingGoto PendingGoto;
struct LabelScope {
    LabelScope *next;
    char *name;
    LVar *locals;
};

static LabelScope *label_scopes;
struct PendingGoto {
    PendingGoto *next;
    char *name;
    Node *node;
};

static PendingGoto *pending_gotos;

// GNU nested functions: per-enclosing-function parser state, pushed before
// parsing a nested function's own params/body (which gets a fresh, empty
// locals/label_scopes/etc of its own — exactly the reset parse()'s
// toplevel loop already does for every top-level function) and popped
// immediately after. The saved `locals`/`label_scopes` lists are frozen
// snapshots (arena-allocated, never mutated after the snapshot) that
// find_var()/goto resolution walk outward through when a name isn't found
// in the current (innermost) scope — this is what lets a nested function
// reference an enclosing function's locals and __label__ labels.
// A `post(NAME: ...)` return-value binding: one real local per distinct
// NAME used by a function's active postconditions (see
// activate_function_contracts()), shared by every postcondition naming it.
typedef struct PostBind PostBind;
struct PostBind {
    PostBind *next;
    char *name;
    LVar *var;
};

typedef struct FnCtx FnCtx;
struct FnCtx {
    FnCtx *next;
    char *fn_name; // enclosing function's name, for asm-name mangling
    LVar *locals;
    int stack_offset;
    LVar *current_fn_scope_locals;
    char *parser_current_fn;
    int current_block_depth;
    LabelScope *label_scopes;
    PendingGoto *pending_gotos;
    Node *current_switch;
    Node *current_loop;
    bool fn_uses_vla;
    bool current_fn_is_inline;
    Contract *current_fn_postconds;
    PostBind *current_fn_postcond_binds;
    LVar *current_fn_range_params; // for the -O3 contract range prover
};
static FnCtx *fn_ctx_stack;
static int fn_ctx_depth;
static int nested_fn_counter; // disambiguates same-named nested fns file-wide
// pre(...)/post(...) contracts active for the function currently being
// parsed (see activate_function_contracts()); NULL when none apply. Only
// stmt()'s "return" handling (apply_postconds_to_return()) consults
// these; preconditions are fully discharged once, at function entry, by
// activate_function_contracts() itself.
static Contract *current_fn_postconds;
static PostBind *current_fn_postcond_binds;
// Every parameter of the function currently being parsed, param_next-
// linked (== the `params` local the file-scope function-definition path
// already builds) — set unconditionally by activate_function_contracts()
// so contract_assert()/contract_assume() (parse_contract_stmt(), which
// has no other route to it) can also feed the -O3 range prover.
static LVar *current_fn_range_params;
// Tentative declaration: the defining declaration (with tl_item_head)
// lives much later in this file, alongside parse()'s toplevel loop that
// initializes it; parse_nested_function_def() (defined earlier in the
// file, right after compound_stmt()) also appends to this same list.
static TLItem *tl_item_cur;

// Push the current per-function parser state and reset to a fresh, empty
// state for parsing a nested function's own params/body.
static void push_fn_ctx(void) {
    FnCtx *c = arena_alloc(sizeof(FnCtx));
    c->fn_name = parser_current_fn;
    c->locals = locals;
    c->stack_offset = stack_offset;
    c->current_fn_scope_locals = current_fn_scope_locals;
    c->parser_current_fn = parser_current_fn;
    c->current_block_depth = current_block_depth;
    c->label_scopes = label_scopes;
    c->pending_gotos = pending_gotos;
    c->current_switch = current_switch;
    c->current_loop = current_loop;
    c->fn_uses_vla = fn_uses_vla;
    c->current_fn_is_inline = current_fn_is_inline;
    c->current_fn_postconds = current_fn_postconds;
    c->current_fn_postcond_binds = current_fn_postcond_binds;
    c->current_fn_range_params = current_fn_range_params;
    c->next = fn_ctx_stack;
    fn_ctx_stack = c;
    fn_ctx_depth++;

    locals = NULL;
    stack_offset = CHAIN_RSP_OFFSET; // reserve [80,96) for the static-chain rbp+rsp slots
    current_fn_scope_locals = NULL;
    current_block_depth = 0;
    label_scopes = NULL;
    pending_gotos = NULL;
    current_switch = NULL;
    current_loop = NULL;
    fn_uses_vla = false;
    current_fn_is_inline = false;
    current_fn_postconds = NULL;
    current_fn_postcond_binds = NULL;
    current_fn_range_params = NULL;
}

// Restore the enclosing function's parser state after a nested function's
// params/body have been fully parsed.
static void pop_fn_ctx(void) {
    FnCtx *c = fn_ctx_stack;
    locals = c->locals;
    stack_offset = c->stack_offset;
    current_fn_scope_locals = c->current_fn_scope_locals;
    parser_current_fn = c->parser_current_fn;
    current_block_depth = c->current_block_depth;
    label_scopes = c->label_scopes;
    pending_gotos = c->pending_gotos;
    current_switch = c->current_switch;
    current_loop = c->current_loop;
    fn_uses_vla = c->fn_uses_vla;
    current_fn_is_inline = c->current_fn_is_inline;
    current_fn_postconds = c->current_fn_postconds;
    current_fn_postcond_binds = c->current_fn_postcond_binds;
    current_fn_range_params = c->current_fn_range_params;
    fn_ctx_stack = c->next;
    fn_ctx_depth--;
}
static Node *conditional(Token **rest, Token *tok);

// Fast token/string-literal comparison (avoids strlen at runtime, op is constant)
#define equalc(tok, op)     ((tok) && (tok)->ptr && (tok)->len == (int)(sizeof(op) - 1) && memcmp((tok)->ptr, op, sizeof(op) - 1) == 0)

// Peek past a single __attribute__((...)) block without consuming tokens.
// Returns the token after the closing )), or NULL if structure doesn't match.
static Token *peek_past_attr(Token *tok) {
    if (!equalc(tok, "__attribute__") && !equalc(tok, "__attribute"))
        return NULL;
    tok = tok->next;
    if (!equalc(tok, "(")) return NULL;
    tok = tok->next;
    if (!equalc(tok, "(")) return NULL;
    tok = tok->next;
    int depth = 1;
    while (depth > 0 && tok->kind != TK_EOF) {
        if (equalc(tok, "(")) depth++;
        else if (equalc(tok, ")"))
            depth--;
        tok = tok->next;
    }
    if (!equalc(tok, ")")) return NULL; // final closing paren
    return tok->next;
}

// All skip() callers in this file use string-literal operators, so use the
// compile-time-length equalc() variant and avoid a run-time strlen().
#define skip(tok, op)                                          \
    ({                                                         \
        Token *_t = (tok);                                     \
        if (!equalc(_t, (op)))                                 \
            error_tok(_t, "expected specific operator");       \
        _t->next;                                              \
    })

static int64_t align_to(int64_t n, int64_t align) {
    return (n + align - 1) & ~(align - 1);
}

static Node *new_node(NodeKind kind, Token *tok) {
    Node *node = arena_alloc(sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = expr;
    return node;
}


// Insert implicit casts for function call arguments to match parameter types
static void cast_funcall_args(Node *call) {
    Type *fty = NULL;
    if (call->lhs) {
        check_type(call->lhs);
        Type *t = call->lhs->ty;
        if (t->kind == TY_PTR && t->base && t->base->kind == TY_FUNC)
            fty = t->base;
    }
    if (!fty || !fty->param_types)
        return;
    // C11 6.5.2.2p6/p7: the number of arguments must match a prototyped
    // (non-variadic) function's parameter list; a variadic function needs
    // at least its named parameters.  Without this check a wrong-arity
    // call like `pthread_setname_np ("a")` against glibc's two-parameter
    // prototype compiled silently, so configure probes that rely on the
    // compile error (Emacs's pthread_setname_np-1-arg test) misdetected
    // the ABI and generated calls that crash at runtime.
    if (!fty->is_oldstyle) {
        int nparams = 0;
        for (Type *p = fty->param_types; p; p = p->param_next)
            nparams++;
        int nargs = 0;
        for (Node *a = call->args; a; a = a->next)
            nargs++;
        if (nargs < nparams || (!fty->is_variadic && nargs > nparams))
            error_tok(call->tok, nargs < nparams ? "too few arguments to function" : "too many arguments to function");
    }
    Type *pt = fty->param_types;
    for (Node **arg = &call->args; *arg && pt; arg = &(*arg)->next, pt = pt->param_next) {
        check_type(*arg);
        if (!(*arg)->ty)
            continue;
        // C23: a nullptr_t parameter may only be passed a null pointer
        // constant or another nullptr_t value (same rule as assigning to
        // a nullptr_t object -- a function call is just parameter
        // initialization). A float or an integer constant broken by an
        // intervening non-integer cast doesn't qualify.
        if (pt->kind == TY_NULLPTR_T && !is_null_value_or_nullptr(*arg))
            error_tok((*arg)->tok, "incompatible type for argument (expected 'nullptr_t')");
        bool arg_float = is_flonum((*arg)->ty);
        bool param_float = is_flonum(pt);
        bool arg_int = is_integer((*arg)->ty);
        bool param_int = is_integer(pt);
        if ((is_complex((*arg)->ty) && is_complex(pt) && (*arg)->ty->size != pt->size) ||
            (arg_int && param_float) || (arg_float && param_int) ||
            (arg_int && param_int && (*arg)->ty->size != pt->size)) {
            Node *cast = new_unary(ND_CAST, *arg, (*arg)->tok);
            cast->ty = arena_alloc(sizeof(Type));
            *cast->ty = *pt;
            cast->next = (*arg)->next;
            *arg = cast;
        }
    }
}

// C23 6.4.4.1: the minimal _BitInt(N) width able to represent `v` (which is
// always >= 0 here - literal tokens never carry a sign, that's a separate
// unary operator applied afterward): the magnitude's bit-length, plus one
// more bit for the sign unless unsigned, floored at the type's minimum
// legal width (1 for unsigned, 2 for signed).
static int bitint_literal_width(uint64_t v, bool is_unsigned) {
    int bits = v ? (64 - __builtin_clzll(v)) : 0;
    if (is_unsigned)
        return bits < 1 ? 1 : bits;
    return bits + 1 < 2 ? 2 : bits + 1;
}

static Node *new_num(int64_t val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    // Determine type from suffix encoded in token text
    char *end = tok->ptr + tok->len;
    bool is_u = false;
    int l_count = 0;
    bool is_bitint = false;
    char *s = end - 1;
    while (s >= tok->ptr) {
        char c = *s;
        if (c == 'u' || c == 'U') {
            is_u = true;
            s--;
        } else if (c == 'l' || c == 'L') {
            l_count++;
            s--;
        } else if ((c == 'b' || c == 'B') && s > tok->ptr &&
                   (s[-1] == 'w' || s[-1] == 'W')) {
            // C23 wb/WB suffix (optionally combined with u/U in either
            // order: 42wb, 42uwb, 42wbu, 42WB, 42UWB, 42WBu, ...).
            is_bitint = true;
            s -= 2;
        } else
            break;
    }
    if (is_bitint) {
        node->ty = bitint_type(bitint_literal_width((uint64_t)val, is_u), is_u);
        return node;
    }
    // C11 6.4.4.1 Table 6 / C23 6.4.4.1p5: every magnitude comparison below
    // MUST use the literal's unsigned bit pattern (uval), never the raw
    // signed `val` — a literal token is never negative by construction
    // (unary minus is a separate later operator), so a huge hex/octal
    // constant whose top bit is set (e.g. 0xffffffffffffffff) is a valid
    // magnitude that merely LOOKS like -1 once reinterpreted as int64_t.
    // Comparing that signed reinterpretation against INT_MIN/INT_MAX (as
    // this ladder used to) makes such a constant spuriously "fit" in
    // `int`, truncating its true width to 4 bytes. A decimal constant's
    // candidate list is signed-only (int, long, long long); octal/hex/C23
    // binary constants (all lexed with a leading '0') additionally offer
    // the unsigned type of the same rank once the signed one overflows,
    // without needing a 'u' suffix.
    bool non_decimal = tok->ptr[0] == '0' && tok->len > 1;
    uint64_t uval = (uint64_t)val;
    bool long_is_32 = ty_long->size == 4; // _WIN32 LLP64; else LP64
    uint64_t long_max = long_is_32 ? 0x7FFFFFFFULL : 0x7FFFFFFFFFFFFFFFULL;
    uint64_t ulong_max = long_is_32 ? 0xFFFFFFFFULL : 0xFFFFFFFFFFFFFFFFULL;
    const uint64_t llong_max = 0x7FFFFFFFFFFFFFFFULL;
    if (l_count >= 2) {
        node->ty = is_u                           ? ty_ullong
            : (uval <= llong_max || !non_decimal) ? ty_llong
                                                  : ty_ullong;
    } else if (l_count == 1) {
        if (is_u)
            node->ty = uval <= ulong_max ? ty_ulong : ty_ullong;
        else if (uval <= long_max)
            node->ty = ty_long;
        else if (non_decimal && uval <= ulong_max)
            node->ty = ty_ulong;
        else if (uval <= llong_max || !non_decimal)
            node->ty = ty_llong;
        else
            node->ty = ty_ullong;
    } else if (is_u) {
        node->ty = uval <= 0xFFFFFFFFULL ? ty_uint
            : uval <= ulong_max          ? ty_ulong
                                         : ty_ullong;
    } else if (uval <= 0x7FFFFFFFULL) {
        node->ty = ty_int;
    } else if (non_decimal && uval <= 0xFFFFFFFFULL) {
        node->ty = ty_uint;
    } else if (uval <= long_max) {
        node->ty = ty_long;
    } else if (non_decimal && uval <= ulong_max) {
        node->ty = ty_ulong;
    } else if (uval <= llong_max || !non_decimal) {
        node->ty = ty_llong;
    } else {
        node->ty = ty_ullong;
    }
    return node;
}

static Node *new_fnum(double fval, Token *tok) {
    Node *node = new_node(ND_FNUM, tok);
    node->fval = fval;
    node->ty = tok->val == 2 ? ty_ldouble : ty_double;
    return node;
}

// A _Decimal32/64/128 literal (1.5df, 2.5dd, 3.5dl, C23 1.5d32/d64/d128):
// fold the token text into its IEEE 754-2008 BID bit pattern at compile
// time via the bundled libbid (linked into rcc itself), and store the bits
// in node->val (32/64-bit) or node->val/val2 (128-bit). The lexer already
// parsed the value as a binary double (tok->fval) and discarded the exact
// decimal spelling; re-parse from tok->ptr (raw token text, digit
// separators and suffix included) so the decimal value is exact.
#ifndef __MUSL__
static Node *new_decimal(Token *tok) {
    // Decode suffix: df/dd/dl (legacy) and d32/d64/d128 (C23). The token
    // text ends with the suffix; the numeric part is everything before.
    char *p = tok->ptr + tok->len; // end of raw text
    Type *ty = NULL;
    if (p - tok->ptr >= 2) {
        if ((p[-2] == 'd' || p[-2] == 'D') && (p[-1] == 'f' || p[-1] == 'F'))
            ty = ty_decimal32;
        else if ((p[-2] == 'd' || p[-2] == 'D') && (p[-1] == 'd' || p[-1] == 'D'))
            ty = ty_decimal64;
        else if ((p[-2] == 'd' || p[-2] == 'D') && (p[-1] == 'l' || p[-1] == 'L'))
            ty = ty_decimal128;
        else if (p - tok->ptr >= 3 && (p[-3] == 'd' || p[-3] == 'D') &&
                 p[-2] == '3' && p[-1] == '2')
            ty = ty_decimal32;
        else if (p - tok->ptr >= 3 && (p[-3] == 'd' || p[-3] == 'D') &&
                 p[-2] == '6' && p[-1] == '4')
            ty = ty_decimal64;
        else if (p - tok->ptr >= 4 && (p[-4] == 'd' || p[-4] == 'D') &&
                 p[-3] == '1' && p[-2] == '2' && p[-1] == '8')
            ty = ty_decimal128;
    }
    if (!ty)
        return NULL; // not a decimal literal (shouldn't happen from lexer)

    // Strip the suffix: df/dd/dl = 2 chars, d32/d64 = 3, d128 = 4
    // (dl is decimal128 but its suffix is only 2 chars). Detect from the
    // actual text, not from the type.
    int suffix = 2;
    if (p - tok->ptr >= 4 && (p[-4] == 'd' || p[-4] == 'D') &&
        p[-3] == '1' && p[-2] == '2' && p[-1] == '8')
        suffix = 4; // d128
    else if (p - tok->ptr >= 3 && (p[-3] == 'd' || p[-3] == 'D') &&
             (p[-2] == '3' || p[-2] == '6') && isdigit(p[-1]))
        suffix = 3; // d32/d64
    char buf[256];
    int n = 0;
    char *end = p - suffix;
    for (char *q = tok->ptr; q < end; q++) {
        if (*q != '\'')
            buf[n++] = *q;
        if (n >= 250)
            break;
    }
    buf[n] = '\0';

    Node *node = new_node(ND_NUM, tok);
    node->ty = ty;
    parser_used_decimal = true;
    if (ty == ty_decimal128) {
        BID_UINT128 r = __bid128_from_string(buf);
        node->val = (int64_t)r.w[0];
        node->val2 = (int64_t)r.w[1];
    } else if (ty == ty_decimal64) {
        node->val = (int64_t)__bid64_from_string(buf);
    } else {
        unsigned long long r = __bid64_from_string(buf);
        node->val = (int64_t)__bid64_to_bid32(r);
    }
    return node;
}
#else
/* musl: no libdfp, return NULL so caller falls back to new_fnum() */
static Node *new_decimal(Token *tok) {
    (void)tok;
    return NULL;
}
#endif

// Compute the byte size expression for a VLA allocation: count * element_size
static Node *vla_alloc_size(Type *ty, Token *tok) {
    Node *base_sz = (ty->base->kind == TY_VLA)
        ? vla_alloc_size(ty->base, tok)
        : new_node(ND_NUM, tok);
    if (ty->base->kind != TY_VLA) {
        base_sz->val = ty->base->size;
        base_sz->ty = size_t_type();
    }
    Node *count = ty->vla_len_expr ? ty->vla_len_expr : new_node(ND_NUM, tok);
    if (!ty->vla_len_expr) {
        count->val = ty->array_len;
        count->ty = size_t_type();
    }
    return new_binary(ND_MUL, count, base_sz, tok);
}

static Type *copy_type(Type *ty) {
    // For struct/union types, return the original. These are identity types
    // that can be completed later via tag declarations. Creating a shallow
    // copy would leave the copy incomplete forever.
    if (ty->kind == TY_STRUCT || ty->kind == TY_UNION)
        return ty;
    Type *ret = arena_alloc(sizeof(Type));
    *ret = *ty;
    return ret;
}

// Qualify a type WITHOUT ever mutating a shared struct/union Type object:
// copy_type() returns the original for every struct/union (see above), so
// `copy_type(ty)->qual |= X` would permanently qualify every other
// declaration of the same struct type in the translation unit.
// Real bug: mimalloc.h forward-declares `struct mi_heap_s` and uses
// `const mi_heap_t*` before the type is completed -- the incomplete-path
// qual stamping const-qualified the shared mi_heap_s type, so the
// NON-const `mi_heap_t _mi_heap_main` in init.c read as const too, and
// eval_const_expr()'s ND_MEMBER fold (correctly gated on a const object)
// folded `_mi_heap_main.thread_id == 0` to TRUE, making
// `_mi_is_main_thread()` return constant 1 -- every thread then shared
// _mi_heap_main, corrupting the multithreaded allocator.
// A complete struct gets a real shallow copy with the qual; an incomplete
// one gets a QUALIFIED VARIANT linked off the canonical type (see
// Type.qual_variants), which struct_or_union_specifier() completes in
// lockstep -- member access, sizeof and declaration-vs-definition type
// compatibility all read through the variant, yet its qualifier never
// leaks onto the canonical type; any other type gets the ordinary copy.
static Type *qualify_struct_type(Type *ty, unsigned char quals) {
    if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
        Type *ret = arena_alloc(sizeof(Type));
        *ret = *ty;
        if (ty->has_body) {
            ret->qual |= quals;
            return ret;
        }
        ret->qual = ty->qual | quals;
        ret->use_qual = quals;
        ret->qual_variants = ty->qual_variants;
        ty->qual_variants = ret;
        return ret;
    }
    ty = copy_type(ty);
    ty->qual |= quals;
    return ty;
}

// C23 constexpr object types: same qualified-copy semantics as
// qualify_struct_type() (never mutate a shared struct/union type).
Type *qualify_type_copy(Type *ty, unsigned char quals) {
    return qualify_struct_type(ty, quals);
}

// The type of a `.`/`->` member-access expression: the member's own
// declared type, plus any const/volatile qualifier the BASE struct/union
// expression carries but the member's own type doesn't (C11 6.5.2.3p3:
// "the result has ... the type qualifiers of the specified member" --
// but accessing a member through a qualified struct/union additionally
// qualifies the result even when the member's own declared type has no
// qualifier at all, e.g. a plain `char buf[N]` member read through a
// `const struct S *`). `base_ty` is the struct/union's own type (already
// dereferenced for `->`); a bitfield narrows/promotes to ty_int/ty_uint
// first (matching real GCC: sizeof/printf-format select the promoted
// width), then that result is qualified the same way.
static Type *member_access_type(Type *base_ty, Member *mem) {
    Type *ty;
    if (mem->bit_width > 0) {
        int bw = mem->bit_width;
        if (bw < 32 || (bw == 32 && !mem->ty->is_unsigned))
            ty = ty_int;
        else if (bw == 32)
            ty = ty_uint;
        else
            ty = mem->ty;
    } else {
        ty = mem->ty;
    }
    unsigned char inherit = base_ty->qual & (QUAL_CONST | QUAL_VOLATILE) & ~ty->qual;
    return inherit ? qualify_type_copy(ty, inherit) : ty;
}

// C23 typeof_unqual: recursively strip all qualifiers from a type.
static Type *type_unqual(Type *ty) {
    if (!ty) return NULL;
    Type *ret = copy_type(ty);
    ret->qual = 0;
    if (ret->base) ret->base = type_unqual(ret->base);
    if (ret->return_ty) ret->return_ty = type_unqual(ret->return_ty);
    return ret;
}

static Type *apply_type_align(Type *ty, int align) {
    // NB: this only raises the alignment *requirement* for this particular
    // declaration (e.g. `_Alignas(16) unsigned short in[N];`), and must
    // NOT pad ty->size — a scalar/array element's sizeof is fixed by the
    // ABI regardless of an over-alignment request on one declared object.
    // A struct/union whose *own* trailing attribute widens its alignment
    // (`struct S { ... } __attribute__((aligned(N)));`, changing the type
    // itself, not just one declaration of it) pads its size separately in
    // struct_or_union_specifier, where that distinction is still visible.
    if (align <= 0 || align <= ty->align)
        return ty;
    // Deliberately not copy_type(): for a *complete* struct/union it
    // intentionally returns the same Type object (so an incomplete
    // forward-declared type can still be completed later through every
    // existing pointer to it) — reusing that here would mutate the shared
    // type's own alignment in place instead of raising just this one
    // declaration's requirement, corrupting every other use of the same
    // struct/union (typedef'd or not, e.g. atomic_long_t) for the rest of
    // the translation unit. Only fall back to sharing identity for a
    // still-incomplete type, where a real clone would freeze size/members
    // at 0 forever and never see the later completion.
    bool incomplete_aggregate = (ty->kind == TY_STRUCT || ty->kind == TY_UNION) &&
        !ty->has_body;
    if (incomplete_aggregate)
        return ty;
    Type *ret = arena_alloc(sizeof(Type));
    *ret = *ty;
    ret->align = align;
    return ret;
}

static Type *func_type(Type *return_ty) {
    Type *ty = arena_alloc(sizeof(Type));
    ty->kind = TY_FUNC;
    ty->size = 1;
    ty->align = 1;
    ty->return_ty = return_ty;
    return ty;
}

static LVar *new_var(char *name, Type *ty, bool is_local) {
    LVar *var = arena_alloc(sizeof(LVar));
    var->name = str_intern(name, strlen(name));
    var->ty = ty;
    var->is_local = is_local;
    var->alias_target = NULL;
    var->asm_name = NULL;

    if (is_local) {
        // VLA-containing struct: ty->size is 0 (runtime size). Like TY_VLA,
        // ND_ALLOCA codegen writes both a restore-SP marker at var->offset and
        // the data pointer at var->offset-8, so reserve 16 bytes (matching
        // TY_VLA's placeholder size below).
        bool vla_struct = (ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->vla_len_expr;
        int size = vla_struct ? 16 : (ty->size < 4 ? 4 : ty->size);
        int align = vla_struct ? 8 : (ty->align < 4 ? 4 : ty->align);
        stack_offset = align_to(stack_offset + size, align);
        var->offset = stack_offset;
        var->next = locals;
        locals = var;
    } else {
        // A block-scope `static` (is_local=false, but declared while
        // parser_current_fn is set) needs to be dropped alongside its
        // enclosing function if that function is ever recognized as
        // dead code — see decl_fn_name's declaration in rcc.h.
        var->decl_fn_name = parser_current_fn;
        var->next = globals;
        globals = var;
        global_htab_add(var);
    }
    return var;
}

static Node *new_var_node(LVar *var, Token *tok) {
    Node *node = new_node(ND_LVAR, tok);
    node->var = var;
    node->ty = var->ty;
    return node;
}

// Create a complex value node from real and imaginary parts.
// Allocates a temporary local variable and returns (setreal, setimag, tmp).
static Node *new_complex_val(Node *real_part, Node *imag_part, Type *cty, Token *tok) {
    LVar *tmp = new_var("", cty, true);
    Node *tmp_var = new_var_node(tmp, tok);
    // __real__ tmp = real_part
    Node *set_real = new_binary(ND_ASSIGN, new_unary(ND_REAL, tmp_var, tok), real_part, tok);
    // __imag__ tmp = imag_part
    Node *set_imag = new_binary(ND_ASSIGN, new_unary(ND_IMAG, tmp_var, tok), imag_part, tok);
    // (set_real, set_imag, tmp)
    Node *comma1 = new_binary(ND_COMMA, set_real, set_imag, tok);
    Node *result = new_binary(ND_COMMA, comma1, tmp_var, tok);
    return result;
}

// Set by find_var() as a side effect on every successful lookup: 0 when
// found in the current function's own `locals` or the global table
// (ordinary access, unchanged from before nested functions existed); N>0
// when found N enclosing-function levels up via the FnCtx stack (see
// Node.chain_depth). Callers that build an ND_LVAR node from a find_var()
// result read this immediately afterward and stash it on the node.
static int last_find_var_chain_depth;

static LVar *find_var(Token *tok) {
    last_find_var_chain_depth = 0;
    for (LVar *var = locals; var; var = var->next)
        if (var->name == tok->name) {
            return var;
        }
    // Nested function: walk outward through enclosing functions' frozen
    // locals snapshots (FnCtx.locals) before falling back to globals, so a
    // nested function body can reference an enclosing function's locals
    // and parameters (GNU nested-function variable capture).
    int depth = 1;
    for (FnCtx *c = fn_ctx_stack; c; c = c->next, depth++) {
        for (LVar *var = c->locals; var; var = var->next)
            if (var->name == tok->name) {
                last_find_var_chain_depth = depth;
                return var;
            }
    }
    return find_global_name(tok->name);
}

LVar *find_global_name(char *name) {
    uint32_t h = hash_name(name) % GLOBAL_HASH_SIZE;
    for (LVar *var = global_htab[h]; var; var = var->hash_next)
        if (var->name == name)
            return var;
    return NULL;
}

// Declare a __builtin_* math function with its proper type signature on first
// use.  Avoids preprocessor macros and strcmp-based return-type tables.
// Returns an ND_LVAR node with the function's type, or NULL if unknown.
static Node *declare_builtin_on_demand(Token *tok) {
    Type *ret_ty = NULL;
    Type *param_tys[3] = {NULL};
    int nparams = 0;

#if 0
    // Every name below starts with "__builtin_", so one length + prefix test
    // rejects all other identifiers before the equalc() chain is walked.
    // Already check before the single call.
    if (!tok->ptr || tok->len <= 10 || memcmp(tok->ptr, "__builtin_", 10))
        return NULL;
#endif

    // float(float, float) group
    if (equalc(tok, "__builtin_powf")) {
        ret_ty = ty_float;
        param_tys[0] = ty_float;
        param_tys[1] = ty_float;
        nparams = 2;
    } else if (equalc(tok, "__builtin_fmaxf")) {
        ret_ty = ty_float;
        param_tys[0] = ty_float;
        param_tys[1] = ty_float;
        nparams = 2;
    } else if (equalc(tok, "__builtin_fminf")) {
        ret_ty = ty_float;
        param_tys[0] = ty_float;
        param_tys[1] = ty_float;
        nparams = 2;
    } else if (equalc(tok, "__builtin_fmaf")) {
        ret_ty = ty_float;
        param_tys[0] = ty_float;
        param_tys[1] = ty_float;
        param_tys[2] = ty_float;
        nparams = 3;
    } else if (equalc(tok, "__builtin_fabsf")) {
        ret_ty = ty_float;
        param_tys[0] = ty_float;
        nparams = 1;
    } else if (equalc(tok, "__builtin_copysignf")) {
        ret_ty = ty_float;
        param_tys[0] = ty_float;
        param_tys[1] = ty_float;
        nparams = 2;
        // double(double, double) group
    } else if (equalc(tok, "__builtin_pow")) {
        ret_ty = ty_double;
        param_tys[0] = ty_double;
        param_tys[1] = ty_double;
        nparams = 2;
    } else if (equalc(tok, "__builtin_fmax")) {
        ret_ty = ty_double;
        param_tys[0] = ty_double;
        param_tys[1] = ty_double;
        nparams = 2;
    } else if (equalc(tok, "__builtin_fmin")) {
        ret_ty = ty_double;
        param_tys[0] = ty_double;
        param_tys[1] = ty_double;
        nparams = 2;
    } else if (equalc(tok, "__builtin_fma")) {
        ret_ty = ty_double;
        param_tys[0] = ty_double;
        param_tys[1] = ty_double;
        param_tys[2] = ty_double;
        nparams = 3;
    } else if (equalc(tok, "__builtin_fabs")) {
        ret_ty = ty_double;
        param_tys[0] = ty_double;
        nparams = 1;
    } else if (equalc(tok, "__builtin_copysign")) {
        ret_ty = ty_double;
        param_tys[0] = ty_double;
        param_tys[1] = ty_double;
        nparams = 2;
    } else if (equalc(tok, "__builtin_nextafter")) {
        ret_ty = ty_double;
        param_tys[0] = ty_double;
        param_tys[1] = ty_double;
        nparams = 2;
    } else if (equalc(tok, "__builtin_nextafterf")) {
        ret_ty = ty_float;
        param_tys[0] = ty_float;
        param_tys[1] = ty_float;
        nparams = 2;
    } else if (equalc(tok, "__builtin_nextafterl")) {
        ret_ty = ty_ldouble;
        param_tys[0] = ty_ldouble;
        param_tys[1] = ty_ldouble;
        nparams = 2;
        // double(double _Complex) group
    } else if (equalc(tok, "__builtin_creal")) {
        ret_ty = ty_double;
        param_tys[0] = complex_type(ty_double);
        nparams = 1;
    } else if (equalc(tok, "__builtin_cimag")) {
        ret_ty = ty_double;
        param_tys[0] = complex_type(ty_double);
        nparams = 1;
        // float(float _Complex) group
    } else if (equalc(tok, "__builtin_crealf")) {
        ret_ty = ty_float;
        param_tys[0] = complex_type(ty_float);
        nparams = 1;
    } else if (equalc(tok, "__builtin_cimagf")) {
        ret_ty = ty_float;
        param_tys[0] = complex_type(ty_float);
        nparams = 1;
    } else if (equalc(tok, "__builtin_bswap16")) {
        ret_ty = ty_ushort;
        param_tys[0] = ty_ushort;
        nparams = 1;
    } else if (equalc(tok, "__builtin_bswap32")) {
        ret_ty = ty_uint;
        param_tys[0] = ty_uint;
        nparams = 1;
    } else if (equalc(tok, "__builtin_bswap64")) {
        ret_ty = ty_ullong;
        param_tys[0] = ty_ullong;
        nparams = 1;
    } else if (equalc(tok, "__builtin_alloca")) {
        // Only reached when the preprocessor's own __builtin_alloca ->
        // alloca object-macro alias (preprocess.c's define_pre) failed
        // to apply: glibc's <alloca.h> defines the *opposite* direction
        // as a function-like macro (`#define alloca(size)
        // __builtin_alloca(size)`), and a TU that pulls it in (directly,
        // or transitively via <stdlib.h>/<stdbit.h> under _GNU_SOURCE)
        // before __builtin_alloca is used via another macro (e.g.
        // <string.h>'s GNU strdupa/strndupa) hits a two-macro ping-pong
        // that the standard's hide-set rule correctly halts, leaving the
        // *original* "__builtin_alloca(...)" spelling unexpanded. With
        // no declaration anywhere for a function literally named
        // "__builtin_alloca" (only "alloca" is ever declared), this call
        // fell through to an implicit-int-returning undeclared-function
        // default; the surrounding `(char *)` cast then sign-extended
        // the (actually 64-bit pointer) result as if it were a 32-bit
        // int, corrupting it. Synthesize the real `void *(size_t)`
        // signature here, matching <alloca.h>'s own prototype, so the
        // call expression's type is correct regardless of which macro
        // direction won the preprocessor ping-pong. codegen.c's
        // gen_funcall separately normalizes call_target so this is
        // still recognized as the alloca stack-adjustment intrinsic,
        // not emitted as an ordinary external call.
        ret_ty = pointer_to(ty_void);
        param_tys[0] = ty_ulong;
        nparams = 1;
    } else {
        return NULL;
    }
    // Build param type list
    Type param_head = {};
    Type *pcur = &param_head;
    for (int i = 0; i < nparams; i++) {
        Type *pt = arena_alloc(sizeof(Type));
        *pt = *param_tys[i];
        pt->param_next = NULL;
        pcur = pcur->param_next = pt;
    }

    Type *fty = func_type(ret_ty);
    fty->param_types = param_head.param_next;

    LVar *var = arena_alloc(sizeof(LVar));
    memset(var, 0, sizeof(LVar));
    var->name = tok->name;
    var->ty = pointer_to(fty);
    var->is_function = true;
    var->is_extern = true;
    global_htab_add(var);

    Node *node = new_var_node(var, tok);
    node->ty = pointer_to(fty);
    return node;
}

// Does this AST (sub)tree contain a __builtin_va_arg_pack[_len]() placeholder?
static bool node_uses_va_arg_pack(Node *n) {
    if (!n)
        return false;
    if (n->kind == ND_VA_ARG_PACK || n->kind == ND_VA_ARG_PACK_LEN)
        return true;
    return node_uses_va_arg_pack(n->lhs) || node_uses_va_arg_pack(n->rhs) ||
        node_uses_va_arg_pack(n->cond) || node_uses_va_arg_pack(n->then) ||
        node_uses_va_arg_pack(n->els) || node_uses_va_arg_pack(n->init) ||
        node_uses_va_arg_pack(n->inc) || node_uses_va_arg_pack(n->body) ||
        node_uses_va_arg_pack(n->args) || node_uses_va_arg_pack(n->next) ||
        node_uses_va_arg_pack(n->stmt_expr_result);
}

static InlinePackFn *find_inline_pack_fn(char *name) {
    for (InlinePackFn *p = inline_pack_fns; p; p = p->next)
        if (strcmp(p->name, name) == 0)
            return p;
    return NULL;
}

// Context for cloning a __builtin_va_arg_pack-using function's body into a
// call site: named parameters are remapped to fresh shadow locals (assigned
// once from the call's arguments), "return EXPR;"/"return;" become
// "{ __pack_ret = EXPR; goto __pack_end; }" / "{ goto __pack_end; }", and
// __builtin_va_arg_pack() argument slots are spliced with the call's
// trailing variadic arguments (shared, not cloned - each call site has its
// own unique copies).
typedef struct InlineCloneCtx {
    LVar **old_params;
    LVar **new_params;
    int nparams;
    Node *pack_args;
    LVar *ret_var;
    char *end_label;
} InlineCloneCtx;

static Node *clone_inline_node(Node *n, InlineCloneCtx *ctx);

static Node *clone_inline_stmts(Node *n, InlineCloneCtx *ctx) {
    Node head = {0};
    Node *tail = &head;
    for (Node *cur = n; cur; cur = cur->next) {
        tail->next = clone_inline_node(cur, ctx);
        tail = tail->next;
    }
    return head.next;
}

static Node *clone_inline_args(Node *n, InlineCloneCtx *ctx) {
    Node head = {0};
    Node *tail = &head;
    for (Node *cur = n; cur; cur = cur->next) {
        if (cur->kind == ND_VA_ARG_PACK) {
            for (Node *p = ctx->pack_args; p; p = p->next) {
                Node *pc = arena_alloc(sizeof(Node));
                *pc = *p;
                pc->next = NULL;
                check_type(pc);
                tail->next = pc;
                tail = tail->next;
            }
            continue;
        }
        tail->next = clone_inline_node(cur, ctx);
        tail = tail->next;
    }
    return head.next;
}

static Node *clone_inline_node(Node *n, InlineCloneCtx *ctx) {
    if (!n)
        return NULL;
    switch (n->kind) {
    case ND_LVAR:
        for (int i = 0; i < ctx->nparams; i++)
            if (n->var == ctx->old_params[i])
                return new_var_node(ctx->new_params[i], n->tok);
        break;
    case ND_VA_ARG_PACK_LEN: {
        // __builtin_va_arg_pack_len() -> number of the call site's trailing
        // variadic args. glibc's fortify open()/openat() inlines test this
        // (e.g. `__va_arg_pack_len() < 1`) to decide whether a mode argument
        // was supplied; a hardcoded 0 wrongly routed open(...,O_CREAT,mode)
        // to __open_2, dropping the mode.
        int n_pack = 0;
        for (Node *p = ctx->pack_args; p; p = p->next)
            n_pack++;
        return new_num(n_pack, n->tok);
    }
    case ND_RETURN: {
        // return EXPR; -> { __pack_ret = EXPR; goto __pack_end; }
        // return;      -> { goto __pack_end; }
        Node *blk = new_node(ND_BLOCK, n->tok);
        Node *head = NULL, **tail = &head;
        if (n->lhs) {
            Node *expr = clone_inline_node(n->lhs, ctx);
            Node *assign = new_binary(ND_ASSIGN, new_var_node(ctx->ret_var, n->tok), expr, n->tok);
            check_type(assign);
            *tail = new_unary(ND_EXPR_STMT, assign, n->tok);
            tail = &(*tail)->next;
        }
        Node *gt = new_node(ND_GOTO, n->tok);
        gt->label_name = ctx->end_label;
        *tail = gt;
        blk->body = head;
        return blk;
    }
    default:
        break;
    }
    Node *c = arena_alloc(sizeof(Node));
    *c = *n;
    c->next = NULL;
    c->lhs = clone_inline_node(n->lhs, ctx);
    c->rhs = clone_inline_node(n->rhs, ctx);
    c->cond = clone_inline_node(n->cond, ctx);
    c->then = clone_inline_node(n->then, ctx);
    c->els = clone_inline_node(n->els, ctx);
    c->init = clone_inline_node(n->init, ctx);
    c->inc = clone_inline_node(n->inc, ctx);
    // stmt_expr_result aliases the lhs of the last ND_EXPR_STMT in body (see
    // the ND_STMT_EXPR construction sites above); codegen locates the
    // value-producing statement via pointer equality against that alias, so
    // find the cloned equivalent instead of cloning stmt_expr_result
    // independently, which would produce a distinct, non-`==` object.
    Node *last_orig = n->body;
    while (last_orig && last_orig->next)
        last_orig = last_orig->next;
    c->body = clone_inline_stmts(n->body, ctx);
    c->args = clone_inline_args(n->args, ctx);
    if (last_orig && last_orig->kind == ND_EXPR_STMT && last_orig->lhs == n->stmt_expr_result) {
        Node *last_clone = c->body;
        while (last_clone && last_clone->next)
            last_clone = last_clone->next;
        c->stmt_expr_result = (last_clone && last_clone->kind == ND_EXPR_STMT) ? last_clone->lhs
                                                                               : clone_inline_node(n->stmt_expr_result, ctx);
    } else {
        c->stmt_expr_result = clone_inline_node(n->stmt_expr_result, ctx);
    }
    return c;
}

// Expand a call to a registered __builtin_va_arg_pack-using function at this
// call site into a GNU statement expression: assign named arguments to
// shadow locals, inline a clone of the callee's body with
// __builtin_va_arg_pack() replaced by the remaining (variadic) arguments,
// and capture the return value (if any) as the expression's result.
static Node *inline_pack_call(Node *call, InlinePackFn *ipf, Token *tok) {
    Function *callee = ipf->fn;
    int nparams = 0;
    for (LVar *p = callee->params; p; p = p->param_next)
        nparams++;

    InlineCloneCtx ctx = {0};
    ctx.nparams = nparams;
    if (nparams > 0) {
        ctx.old_params = arena_alloc(sizeof(LVar *) * nparams);
        ctx.new_params = arena_alloc(sizeof(LVar *) * nparams);
    }

    int id = inline_pack_counter++;

    Node head_stmts = {0};
    Node *tail = &head_stmts;

    Node *cur = call->args;
    int i = 0;
    for (LVar *p = callee->params; p; p = p->param_next, i++) {
        ctx.old_params[i] = p;
        LVar *shadow = new_var(format("__pack_arg%d_%d", id, i), copy_type(p->ty), true);
        ctx.new_params[i] = shadow;
        Node *arg = cur ? cur : new_num(0, tok);
        Node *assign = new_binary(ND_ASSIGN, new_var_node(shadow, tok), arg, tok);
        check_type(assign);
        tail->next = new_unary(ND_EXPR_STMT, assign, tok);
        tail = tail->next;
        if (cur)
            cur = cur->next;
    }
    ctx.pack_args = cur;

    Type *ret_ty = callee->ty->return_ty;
    LVar *ret_var = (ret_ty->kind != TY_VOID) ? new_var(format("__pack_ret%d", id), copy_type(ret_ty), true) : NULL;
    ctx.ret_var = ret_var;
    ctx.end_label = format("__pack_end%d", id);

    tail->next = clone_inline_stmts(callee->body, &ctx);
    while (tail->next)
        tail = tail->next;

    Node *label = new_node(ND_LABEL, tok);
    label->label_name = ctx.end_label;
    label->lhs = new_node(ND_NULL, tok);
    tail->next = label;
    tail = label;

    Node *se = new_node(ND_STMT_EXPR, tok);
    if (ret_var) {
        Node *ret_node = new_var_node(ret_var, tok);
        tail->next = new_unary(ND_EXPR_STMT, ret_node, tok);
        tail = tail->next;
        se->stmt_expr_result = ret_node;
        se->ty = ret_node->ty;
    } else {
        se->ty = ty_void;
    }
    se->body = head_stmts.next;
    return se;
}

static LabelScope *find_label_scope(char *name) {
    for (LabelScope *ls = label_scopes; ls; ls = ls->next)
        if (ls->name == name)
            return ls;
    return NULL;
}

// Nested-function &&label / nonlocal-goto support: find which enclosing
// function (by name) declares `name` as a label, walking outward through
// the FnCtx stack's frozen label_scopes snapshots after a miss in the
// current function's own label_scopes. Returns NULL (and leaves
// *owner_fn_depth at 0) when the label belongs to the CURRENT function
// (ordinary case, unchanged from before nested functions existed) or
// isn't found anywhere; otherwise returns the owning function's name and
// sets *owner_fn_depth to how many FnCtx levels up it was found (mirrors
// find_var()'s chain_depth, though label references don't need the value
// at codegen time the way variable chain-walks do — a label's address is
// a compile-time-constant local symbol regardless of which function
// defines it).
static char *find_label_scope_owner(char *name, int *owner_fn_depth) {
    *owner_fn_depth = 0;
    if (find_label_scope(name))
        return NULL; // declared in the current function: ordinary case
    int depth = 1;
    for (FnCtx *c = fn_ctx_stack; c; c = c->next, depth++) {
        for (LabelScope *ls = c->label_scopes; ls; ls = ls->next)
            if (ls->name == name) {
                *owner_fn_depth = depth;
                return c->fn_name;
            }
    }
    return NULL;
}

// Set of function names (interned pointers) that are the target of an
// actual nonlocal goto from a nested descendant - i.e. functions whose
// codegen prologue must record its own stable rsp/sp (CHAIN_RSP_OFFSET
// in rcc.h) for that descendant's ND_GOTO to read back. Populated during
// parsing (see the goto-statement handler below); parsing completes
// fully before codegen runs (see lib.c/main.c: parse() then codegen()),
// so by the time codegen's prologue-emission code queries this via
// is_goto_target_fn(), the set is final. Taking a label's *address*
// (&&label / ND_LABEL_VAL) does NOT need this - only an actual jump
// restores stack state.
typedef struct GotoTargetFn GotoTargetFn;
struct GotoTargetFn {
    char *name;
    GotoTargetFn *next;
};
static GotoTargetFn *goto_target_fns;

static void mark_goto_target_fn(char *name) {
    for (GotoTargetFn *g = goto_target_fns; g; g = g->next)
        if (g->name == name) return;
    GotoTargetFn *g = arena_alloc(sizeof(GotoTargetFn));
    g->name = name;
    g->next = goto_target_fns;
    goto_target_fns = g;
}

bool is_goto_target_fn(const char *name) {
    for (GotoTargetFn *g = goto_target_fns; g; g = g->next)
        if (g->name == name) return true;
    return false;
}

static void record_label_scope(char *name, LVar *locals_at_label) {
    LabelScope *ls = find_label_scope(name);
    if (ls) {
        ls->locals = locals_at_label;
        return;
    }

    ls = arena_alloc(sizeof(LabelScope));
    ls->name = name;
    ls->locals = locals_at_label;
    ls->next = label_scopes;
    label_scopes = ls;
}

static void add_pending_goto(char *name, Node *node) {
    PendingGoto *pg = arena_alloc(sizeof(PendingGoto));
    pg->name = name;
    pg->node = node;
    pg->next = pending_gotos;
    pending_gotos = pg;
}

static void resolve_pending_gotos(char *name, LVar *locals_at_label) {
    for (PendingGoto *pg = pending_gotos; pg; pg = pg->next) {
        if (pg->name == name)
            pg->node->cleanup_end = locals_at_label;
    }
}

static Typedef *find_typedef(Token *tok) {
    if (!tok || !tok->name) return NULL;
    uint32_t h = hash_name(tok->name) % SCOPE_HASH_SIZE;
    for (Typedef *td = typedef_htab[h]; td; td = td->hash_next)
        if (td->name == tok->name)
            return td;
    return NULL;
}

void add_typedef(char *name, Type *ty) {
    Typedef *td = arena_alloc(sizeof(Typedef));
    td->name = name;
    td->ty = ty;
    td->next = typedefs;
    typedefs = td;
    typedef_htab_add(td);
}

void init_builtins(void) {
    add_typedef("wchar_t",
#ifdef _WIN32
                ty_ushort
#else
                ty_uint
#endif
    );
    add_typedef("iconv_t", pointer_to(ty_void));
    // GCC builtin 16-bit float types. rcc has no true 16-bit float
    // codegen (and no AVX512-BF16/FP16 instruction set), so model them
    // as 16-bit storage: the typedefs in the real GCC headers
    // (avx512bf16vlintrin.h's `typedef __bf16 __v16bf ...`) then parse,
    // and any actual bf16/fp16 arithmetic falls through the normal
    // integer paths instead of erroring on an unknown type name. The
    // size-2 vectors these produce (32/64-byte __m256bh/__m512bh) are
    // already supported by the vector_size machinery.
    add_typedef(str_intern("__bf16", 6), ty_ushort);
    add_typedef(str_intern("_Float16", 8), ty_ushort);
}

static Type *typedef_find_name(const char *name) {
    Token tok = {};
    tok.name = str_intern(name, strlen(name));
    Typedef *td = find_typedef(&tok);
    return td ? td->ty : NULL;
}

static EnumConst *find_enum_const(Token *tok) {
    if (!tok || !tok->name) return NULL;
    uint32_t h = hash_name(tok->name) % SCOPE_HASH_SIZE;
    for (EnumConst *ec = enum_htab[h]; ec; ec = ec->hash_next)
        if (ec->name == tok->name)
            return ec;
    return NULL;
}

static TagScope *find_tag(Token *tok) {
    if (!tok || !tok->name) return NULL;
    uint32_t h = hash_name(tok->name) % SCOPE_HASH_SIZE;
    for (TagScope *tag = tag_htab[h]; tag; tag = tag->hash_next)
        if (tag->name == tok->name)
            return tag;
    return NULL;
}

static TagScope *push_tag(char *name, Type *ty) {
    TagScope *tag = arena_alloc(sizeof(TagScope));
    tag->name = name;
    tag->ty = ty;
    tag->depth = current_block_depth;
    tag->next = tags;
    tags = tag;
    tag_htab_add(tag);
    return tag;
}

static Member *find_member_by_name(Type *ty, char *name) {
    if (ty->kind != TY_STRUCT && ty->kind != TY_UNION)
        return NULL;
    name = str_intern(name, strlen(name));
    for (Member *mem = ty->members; mem; mem = mem->next) {
        if (!mem->name) {
            // Anonymous struct/union: search inside recursively
            if (mem->ty && (mem->ty->kind == TY_STRUCT || mem->ty->kind == TY_UNION)) {
                Member *found = find_member_by_name(mem->ty, name);
                if (found) {
                    // Return synthetic member with combined offset
                    Member *syn = arena_alloc(sizeof(Member));
                    *syn = *found;
                    syn->offset += mem->offset;
                    return syn;
                }
            }
            continue;
        }
        if (mem->name == name)
            return mem;
    }
    return NULL;
}

static Member *find_member(Type *ty, Token *tok) {
    if (ty->kind != TY_STRUCT && ty->kind != TY_UNION)
        error_tok(tok, "not a struct or union");
    return find_member_by_name(ty, tok->name);
}

static StrLit *new_str_lit(char *str, int len, int prefix, int elem_size) {
    StrLit *s = arena_alloc(sizeof(StrLit));
    s->str = str;
    s->len = len;
    s->id = str_lit_counter++;
    s->prefix = prefix;
    s->elem_size = elem_size;
    s->next = str_lits;
    str_lits = s;
    return s;
}

// Element size in bytes for a string literal of the given prefix -- must
// agree with primary()'s TK_STR type-selection switch (and its
// new_str_lit() call using node->ty->base->size) so a string used purely
// for its ADDRESS (pointer-typed global/const initializer, "&expr" reloc
// extraction) records the SAME element size as one used as a value
// expression. codegen.c's wide-string emission pads each literal's start
// to elem_size before writing its bytes (glibc's vectorized wcslen/wmemcmp
// assume any wchar_t object satisfies _Alignof(wchar_t)); a call site that
// under-reports elem_size (e.g. always 1) skips that padding, so the
// literal can land misaligned and any libc wide-string function reading it
// silently returns wrong results.
static int str_lit_elem_size(int prefix) {
    switch (prefix) {
    case 'L':
#ifdef _WIN32
        return 2; // Windows wchar_t: UTF-16
#else
        return 4; // Linux/macOS wchar_t: UTF-32
#endif
    case 'u': return 2; // char16_t: always 16-bit
    case 'U': return 4; // char32_t: always 32-bit
    default: return 1; // plain / u8
    }
}

// Reconstruct a contract condition's source text (best effort — only
// meaningful when the condition wasn't itself produced by macro
// expansion, i.e. its tokens are contiguous in one source buffer) for the
// runtime-violation diagnostic built by make_contract_fail_block().
static char *contract_cond_text(Token *start, Token *end) {
    if (start == end || !start->ptr)
        return "<condition>";
    Token *last = start;
    while (last->next && last->next != end)
        last = last->next;
    if (!last->ptr || last->ptr < start->ptr)
        return "<condition>";
    int len = (int)(last->ptr - start->ptr) + last->len;
    if (len <= 0 || len > 2048)
        return "<condition>";
    return format("%.*s", len, start->ptr);
}

// { write(2, "<diagnostic>\n", N); abort(); } — a freestanding failure
// action that needs neither <stdio.h> nor <stdlib.h> to be included by
// the user's translation unit (unlike fprintf(stderr, ...), stderr is a
// libc-specific macro/symbol on several targets this compiler supports;
// write(2, ...) and abort() are plain, ABI-stable extern symbols on every
// one of them, exactly like declare_builtin_on_demand()'s math-function
// calls or __builtin_clear_padding's synthesized memset() call above).
// Find (or declare) a plain `RET NAME(...)` extern function symbol by
// name, for a compiler-synthesized call — the same pattern
// declare_builtin_on_demand() uses for e.g. libm functions (proven
// across every codegen backend), rather than a bare Node.funcname with
// no Node.lhs (which only a couple of special-cased builtins rely on).
static LVar *get_or_declare_extern_fn(const char *name, Type *ret_ty, Type **param_tys, int nparams) {
    char *iname = str_intern(name, (int)strlen(name));
    LVar *fn = find_global_name(iname);
    if (fn)
        return fn;
    Type param_head = {0};
    Type *pcur = &param_head;
    for (int i = 0; i < nparams; i++) {
        Type *pt = arena_alloc(sizeof(Type));
        *pt = *param_tys[i];
        pt->param_next = NULL;
        pcur = pcur->param_next = pt;
    }
    Type *fty = func_type(ret_ty);
    fty->param_types = param_head.param_next;
    fn = arena_alloc(sizeof(LVar));
    fn->name = iname;
    fn->ty = pointer_to(fty);
    fn->is_function = true;
    fn->is_extern = true;
    global_htab_add(fn);
    return fn;
}

// { write(2, "<diagnostic>\n", N); abort(); } for any contract-violation
// kind (pre/post/contract_assert) — `detail` is either the condition's
// own source text or a user-supplied message (contract_assert's
// optional second argument).
static Node *make_violation_block(const char *kind, const char *detail, char *fn_name,
                                  Token *loc_tok, Token *tok) {
    char *msg = format("rcc: %s '%s' violated in function '%s' at %s:%d\n",
                       kind, detail, fn_name ? fn_name : "?",
                       loc_tok->filename ? loc_tok->filename : "?", loc_tok->lineno);
    int msg_len = (int)strlen(msg);

    Node *strnode = new_node(ND_STR, tok);
    strnode->ty = array_of(ty_char, msg_len + 1);
    StrLit *sl = new_str_lit(msg, msg_len, 0, 1);
    strnode->str_id = sl->id;

    Node *fd = new_num(2, tok);
    fd->ty = ty_int;
    Node *len = new_num(msg_len, tok);
    len->ty = ty_long;
    fd->next = strnode;
    strnode->next = len;

    Type *write_params[3] = {ty_int, pointer_to(ty_char), ty_long};
    LVar *write_fn = get_or_declare_extern_fn("write", ty_long, write_params, 3);
    Node *wcall = new_node(ND_FUNCALL, tok);
    wcall->lhs = new_var_node(write_fn, tok);
    wcall->args = fd;
    wcall->ty = ty_long;

    LVar *abort_fn = get_or_declare_extern_fn("abort", ty_void, NULL, 0);
    Node *acall = new_node(ND_FUNCALL, tok);
    acall->lhs = new_var_node(abort_fn, tok);
    acall->ty = ty_void;

    Node *s1 = new_unary(ND_EXPR_STMT, wcall, tok);
    Node *s2 = new_unary(ND_EXPR_STMT, acall, tok);
    s1->next = s2;

    Node *blk = new_node(ND_BLOCK, tok);
    blk->body = s1;
    return blk;
}

static Node *make_contract_fail_block(Contract *c, bool is_pre, char *fn_name, Token *tok) {
    return make_violation_block(is_pre ? "precondition" : "postcondition",
                                contract_cond_text(c->cond_start, c->cond_end),
                                fn_name, c->tok, tok);
}

// `__builtin_unreachable();` as a statement — reaching it is undefined
// behaviour (codegen.c emits no code and treats what follows as dead).
// This is contract_assume(COND)'s failure action: Gustedt's proposal
// defines contract_assume as `if (!(COND)) unreachable();` verbatim.
static Node *make_unreachable_stmt(Token *tok) {
    Node *call = new_node(ND_FUNCALL, tok);
    call->funcname = str_intern("__builtin_unreachable", 21);
    call->ty = ty_void;
    return new_unary(ND_EXPR_STMT, call, tok);
}

// `if (!(cond)) { <fail> }` for one already-built, already-check_type'd
// contract condition.
static Node *make_contract_check(Node *cond, Contract *c, bool is_pre, char *fn_name, Token *tok) {
    Node *notcond = new_unary(ND_NOT, cond, tok);
    check_type(notcond);
    Node *node = new_node(ND_IF, tok);
    node->cond = notcond;
    node->then = make_contract_fail_block(c, is_pre, fn_name, tok);
    return node;
}

// -----------------------------------------------------------------------
// Contract range prover (-O3 only; see docs/rcc.md and issue #45).
//
// A cheap, in-tree, no-external-dependency second pass beyond
// eval_const_expr()'s literal-constant fold: propagates each in-scope
// variable's *declared type's own* value range (never anything about
// its actual runtime value — that's what makes this sound with zero
// interprocedural/caller analysis: whatever a caller passes for e.g. an
// `unsigned char` parameter is provably in [0,255], full stop) through
// straight-line integer arithmetic via interval analysis (abstract
// interpretation, interval domain). Deliberately does NOT reason about
// floating point at all — NaN/inf make "obviously true" float facts
// unsound without a real IEEE-754 SMT theory (see ESBMC's QF_FP/
// bit-blasting split for what that actually takes) — matching the
// original issue's "much less for floats". Also does not track
// arbitrary local-variable dataflow beyond a variable's own declared
// type: no SSA renaming, no escape/alias analysis, because neither is
// needed for this scope (a real interprocedural SMT prover, e.g. an
// opt-in Z3 backend, is a materially different and separate feature).
//
// Always conservative: "can't decide" (Range.valid == false, or a
// decided-but-mixed [0,1] truth range) falls straight through to the
// existing constant-fold-or-runtime-check path, exactly as if -O3 had
// not been passed. A decided answer never changes program meaning, only
// whether a no-longer-needed runtime check gets elided (provably true)
// or promoted to the same static_assert-style compile error already
// used for a literal `pre(0)` (provably false for every value the
// operand's type can ever hold).
typedef struct {
    bool valid;
    int64_t lo, hi;
} Range;

typedef struct RangeBind RangeBind;
struct RangeBind {
    RangeBind *next;
    LVar *var;
    Range range;
};

static const Range RANGE_UNKNOWN = {false, 0, 0};

static Range range_point(int64_t v) {
    Range r = {true, v, v};
    return r;
}

// The full value range implied by an integer type's own width and
// signedness alone. Bails out on non-integer types and >=64-bit widths
// (unsigned 64-bit's true upper bound doesn't fit an int64_t; a wider
// _BitInt similarly) rather than risk the analyzer's own bookkeeping
// overflowing — never a soundness bug, just fewer conditions decided.
static Range type_range(Type *ty) {
    if (!ty || !is_integer(ty) || ty->size <= 0 || ty->size > 8)
        return RANGE_UNKNOWN;
    int bits = (int)ty->size * 8;
    if (ty->is_unsigned) {
        if (bits >= 64)
            return RANGE_UNKNOWN;
        Range r = {true, 0, ((int64_t)1 << bits) - 1};
        return r;
    }
    if (bits >= 64) {
        Range r = {true, INT64_MIN, INT64_MAX};
        return r;
    }
    Range r = {true, -((int64_t)1 << (bits - 1)), ((int64_t)1 << (bits - 1)) - 1};
    return r;
}

static Range lookup_range(RangeBind *env, LVar *var) {
    for (RangeBind *b = env; b; b = b->next)
        if (b->var == var)
            return b->range;
    return RANGE_UNKNOWN;
}

// Every parameter's own declared-type range (see type_range()). Sound
// for ANY LVar regardless of whether it's a formal parameter or an
// already-initialized local — "somewhere in [type-min, type-max]" holds
// trivially either way — but only parameters (param_next-linked) are
// ever passed in: locals aren't dataflow-tracked (see the section
// comment above), so binding one would just waste an entry.
static RangeBind *build_range_env(LVar *params) {
    RangeBind head = {0};
    RangeBind *cur = &head;
    for (LVar *p = params; p; p = p->param_next) {
        RangeBind *b = arena_alloc(sizeof(RangeBind));
        b->var = p;
        b->range = type_range(p->ty);
        b->next = NULL;
        cur = cur->next = b;
    }
    return head.next;
}

static Range range_neg(Range a) {
    if (!a.valid)
        return RANGE_UNKNOWN;
    __int128 lo = -(__int128)a.hi, hi = -(__int128)a.lo;
    if (lo < INT64_MIN || hi > INT64_MAX)
        return RANGE_UNKNOWN;
    Range r = {true, (int64_t)lo, (int64_t)hi};
    return r;
}

static Range range_add(Range a, Range b) {
    if (!a.valid || !b.valid)
        return RANGE_UNKNOWN;
    __int128 lo = (__int128)a.lo + b.lo, hi = (__int128)a.hi + b.hi;
    if (lo < INT64_MIN || hi > INT64_MAX)
        return RANGE_UNKNOWN;
    Range r = {true, (int64_t)lo, (int64_t)hi};
    return r;
}

static Range range_sub(Range a, Range b) {
    if (!a.valid || !b.valid)
        return RANGE_UNKNOWN;
    __int128 lo = (__int128)a.lo - b.hi, hi = (__int128)a.hi - b.lo;
    if (lo < INT64_MIN || hi > INT64_MAX)
        return RANGE_UNKNOWN;
    Range r = {true, (int64_t)lo, (int64_t)hi};
    return r;
}

static Range range_mul(Range a, Range b) {
    if (!a.valid || !b.valid)
        return RANGE_UNKNOWN;
    __int128 p1 = (__int128)a.lo * b.lo, p2 = (__int128)a.lo * b.hi;
    __int128 p3 = (__int128)a.hi * b.lo, p4 = (__int128)a.hi * b.hi;
    __int128 lo = p1, hi = p1;
    if (p2 < lo) lo = p2;
    if (p2 > hi) hi = p2;
    if (p3 < lo) lo = p3;
    if (p3 > hi) hi = p3;
    if (p4 < lo) lo = p4;
    if (p4 > hi) hi = p4;
    if (lo < INT64_MIN || hi > INT64_MAX)
        return RANGE_UNKNOWN;
    Range r = {true, (int64_t)lo, (int64_t)hi};
    return r;
}

// C truthiness of a range: 1 = every value in it is nonzero (definitely
// true), -1 = it's exactly {0} (definitely false), 0 = straddles zero
// or unknown (can't decide).
static int range_truthiness(Range r) {
    if (!r.valid)
        return 0;
    if (r.lo == 0 && r.hi == 0)
        return -1;
    if (r.lo > 0 || r.hi < 0)
        return 1;
    return 0;
}

// Interval abstract interpretation over one condition expression. Only
// the node kinds relevant to a scalar conditional-expression are
// handled (arithmetic +/-/*, unary -/!, relational/equality, &&/||,
// casts, literals, variable reads); anything else — division, shifts,
// bitwise ops, calls, member/array access, ?: — falls back to
// RANGE_UNKNOWN rather than risk an unsound approximation (C's
// truncating division and shift-of-negative UB in particular are not
// worth the complexity for this budget).
static Range compute_range(Node *n, RangeBind *env) {
    if (!n)
        return RANGE_UNKNOWN;
    switch (n->kind) {
    case ND_NUM:
        return range_point(n->val);
    case ND_LVAR:
        return n->var ? lookup_range(env, n->var) : RANGE_UNKNOWN;
    case ND_CAST: {
        Range a = compute_range(n->lhs, env);
        Range dst = type_range(n->ty);
        if (a.valid && dst.valid && a.lo >= dst.lo && a.hi <= dst.hi)
            return a; // value-preserving cast (e.g. widening)
        return dst; // narrowing (or unknown source): fall back to the destination type's own range
    }
    case ND_NEG:
        return range_neg(compute_range(n->lhs, env));
    case ND_NOT: {
        int t = range_truthiness(compute_range(n->lhs, env));
        if (t == 1) return range_point(0);
        if (t == -1) return range_point(1);
        Range r = {true, 0, 1};
        return r;
    }
    case ND_ADD:
        return range_add(compute_range(n->lhs, env), compute_range(n->rhs, env));
    case ND_SUB:
        return range_sub(compute_range(n->lhs, env), compute_range(n->rhs, env));
    case ND_MUL:
        return range_mul(compute_range(n->lhs, env), compute_range(n->rhs, env));
    case ND_LT: {
        Range a = compute_range(n->lhs, env), b = compute_range(n->rhs, env);
        if (!a.valid || !b.valid) return RANGE_UNKNOWN;
        if (a.hi < b.lo) return range_point(1);
        if (a.lo >= b.hi) return range_point(0);
        Range r = {true, 0, 1};
        return r;
    }
    case ND_LE: {
        Range a = compute_range(n->lhs, env), b = compute_range(n->rhs, env);
        if (!a.valid || !b.valid) return RANGE_UNKNOWN;
        if (a.hi <= b.lo) return range_point(1);
        if (a.lo > b.hi) return range_point(0);
        Range r = {true, 0, 1};
        return r;
    }
    case ND_EQ: {
        Range a = compute_range(n->lhs, env), b = compute_range(n->rhs, env);
        if (!a.valid || !b.valid) return RANGE_UNKNOWN;
        if (a.hi < b.lo || a.lo > b.hi) return range_point(0);
        if (a.lo == a.hi && b.lo == b.hi && a.lo == b.lo) return range_point(1);
        Range r = {true, 0, 1};
        return r;
    }
    case ND_NE: {
        Range a = compute_range(n->lhs, env), b = compute_range(n->rhs, env);
        if (!a.valid || !b.valid) return RANGE_UNKNOWN;
        if (a.hi < b.lo || a.lo > b.hi) return range_point(1);
        if (a.lo == a.hi && b.lo == b.hi && a.lo == b.lo) return range_point(0);
        Range r = {true, 0, 1};
        return r;
    }
    case ND_LOGAND: {
        int lt = range_truthiness(compute_range(n->lhs, env));
        if (lt == -1) return range_point(0);
        int rt = range_truthiness(compute_range(n->rhs, env));
        if (rt == -1) return range_point(0);
        if (lt == 1 && rt == 1) return range_point(1);
        Range r = {true, 0, 1};
        return r;
    }
    case ND_LOGOR: {
        int lt = range_truthiness(compute_range(n->lhs, env));
        if (lt == 1) return range_point(1);
        int rt = range_truthiness(compute_range(n->rhs, env));
        if (rt == 1) return range_point(1);
        if (lt == -1 && rt == -1) return range_point(0);
        Range r = {true, 0, 1};
        return r;
    }
    default:
        return RANGE_UNKNOWN;
    }
}

typedef enum { PROVE_UNKNOWN,
               PROVE_TRUE,
               PROVE_FALSE } ProveResult;

// Entry point: try to decide `cond` from `env` alone. NULL/no-op unless
// -O3 (opt_O3) is active — this whole pass costs nothing at any other
// optimization level, matching -O3's existing "extra, slower analysis"
// convention in real compilers.
static ProveResult try_prove_range(Node *cond, RangeBind *env) {
    if (!opt_O3)
        return PROVE_UNKNOWN;
    int t = range_truthiness(compute_range(cond, env));
    if (t == 1) return PROVE_TRUE;
    if (t == -1) return PROVE_FALSE;
    return PROVE_UNKNOWN;
}

// contract_assert(COND[, "msg"]); / contract_assume(COND[, "msg"]);
// (Gustedt's "Contracts for C" primitives — issue #45's statement forms,
// added alongside the declarator-trailing pre()/post() specifiers:
// https://gustedt.wordpress.com/2025/03/10/contracts-for-c/).
// contract_assert is an always-on assert(): true -> nothing; false ->
// the same diagnostic-then-abort() as a violated pre()/post(). NDEBUG
// never disables it (unlike <assert.h>'s assert()).
// contract_assume promises COND holds without checking it: reaching
// this point with COND false is undefined behaviour, defined verbatim
// by the proposal as `if (!(COND)) unreachable();` — rcc has no
// assumption-propagating optimizer to exploit this for, so it compiles
// to exactly that check (a real branch to __builtin_unreachable(), not
// a no-op), the correctly-defined fallback for a compiler that can't
// yet use the promise.
static Node *parse_contract_stmt(Token **rest, Token *tok, bool is_assert) {
    const char *kw = is_assert ? "contract_assert" : "contract_assume";
    Token *kw_tok = tok;
    tok = skip(tok->next, "(");
    Token *cond_start = tok;
    Node *cond = conditional(&tok, tok);
    check_type(cond);
    Token *cond_end = tok;
    char *msg = NULL;
    if (equalc(tok, ",")) {
        tok = tok->next;
        if (tok->kind != TK_STR)
            error_tok(tok, "expected a string literal message in '%s(...)'", kw);
        msg = tok->str;
        tok = tok->next;
    }
    tok = skip(tok, ")");
    *rest = skip(tok, ";");

    char *detail = msg ? msg : contract_cond_text(cond_start, cond_end);
    long long cval;
    if (eval_const_expr(cond, &cval)) {
        if (cval)
            return new_node(ND_NULL, kw_tok); // provably holds: no-op either way
        if (is_assert)
            error_tok(kw_tok, "%s(%s) is never satisfied", kw, detail);
        // contract_assume with a constant-false condition: unconditionally
        // unreachable, matching the macro's own unreachable() expansion --
        // not a compile error (no more than a bare __builtin_unreachable()
        // itself is), but worth flagging: everything after this point in
        // the enclosing block is now dead code (see codegen.c's
        // bi_unreachable handling) that silently vanishes at -O1+.
        if (!opt_Wno_contract_assume_false)
            warn_tok(kw_tok, "%s(%s) can never hold; code after this point is unreachable and will be eliminated",
                     kw, detail);
        return make_unreachable_stmt(kw_tok);
    }

    ProveResult pr = opt_O3 ? try_prove_range(cond, build_range_env(current_fn_range_params))
                            : PROVE_UNKNOWN;
    if (pr == PROVE_TRUE)
        return new_node(ND_NULL, kw_tok);
    if (pr == PROVE_FALSE) {
        if (is_assert)
            error_tok(kw_tok, "%s(%s) can never be satisfied for any in-range value", kw, detail);
        if (!opt_Wno_contract_assume_false)
            warn_tok(kw_tok,
                     "%s(%s) can never hold for any in-range value; "
                     "code after this point is unreachable and will be eliminated",
                     kw, detail);
        return make_unreachable_stmt(kw_tok);
    }

    Node *notcond = new_unary(ND_NOT, cond, kw_tok);
    check_type(notcond);
    Node *node = new_node(ND_IF, kw_tok);
    node->cond = notcond;
    node->then = is_assert ? make_violation_block(kw, detail, parser_current_fn, kw_tok, kw_tok)
                           : make_unreachable_stmt(kw_tok);
    return node;
}

// Compile one contract condition (replaying its captured, still-unparsed
// token span) and either discharge it statically (constant-true: no
// runtime code at all; constant-false: a static_assert-style compile
// error — Gustedt's proposal explicitly specifies this fast path) or
// return its runtime `if (!(cond)) fail();` check. Returns NULL for the
// constant-true case.
static Node *compile_one_contract(Contract *c, bool is_pre, char *fn_name, Token *tok, RangeBind *range_env) {
    Token *tmp;
    Node *cond = conditional(&tmp, c->cond_start);
    check_type(cond);
    long long cval;
    if (eval_const_expr(cond, &cval)) {
        if (!cval)
            error_tok(c->tok, "%s '%s' is never satisfied",
                      is_pre ? "precondition" : "postcondition",
                      contract_cond_text(c->cond_start, c->cond_end));
        return NULL;
    }
    ProveResult pr = try_prove_range(cond, range_env);
    if (pr == PROVE_TRUE)
        return NULL;
    if (pr == PROVE_FALSE)
        error_tok(c->tok, "%s '%s' can never be satisfied for any in-range value",
                  is_pre ? "precondition" : "postcondition",
                  contract_cond_text(c->cond_start, c->cond_end));
    return make_contract_check(cond, c, is_pre, fn_name, tok);
}

// Called once per function DEFINITION (never for a prototype-only
// declaration, which has nothing to instrument), right after its real,
// final parameter LVars are established in `locals` and before its body
// is parsed. Returns the precondition-check statement chain to prepend
// to the body (NULL if none), and populates current_fn_postconds /
// current_fn_postcond_binds for stmt()'s "return" handling
// (apply_postconds_to_return()) to consult while parsing the body.
//
// Limitation (documented, not a bug): contracts are recognized only on
// the declarator of the definition itself — a separate prototype
// declaration's own pre()/post() clauses (e.g. in a header) are not
// merged in. Repeat them on the definition to enforce them.
static Node *activate_function_contracts(Type *fty, char *fn_name, LVar *params, Token *tok) {
    current_fn_postconds = NULL;
    current_fn_postcond_binds = NULL;
    current_fn_range_params = params;
    if (!fty->preconds && !fty->postconds)
        return NULL;

    // post(NAME: ...) return-value bindings: one real local per distinct
    // NAME, created now (this definition's own stack_offset/locals are
    // correctly set up at this point) and shared by every postcondition
    // naming it.
    PostBind bind_head = {0};
    PostBind *bind_cur = &bind_head;
    for (Contract *c = fty->postconds; c; c = c->next) {
        if (!c->bind_name)
            continue;
        if (fty->return_ty && fty->return_ty->kind == TY_VOID)
            error_tok(c->tok, "'post(%s: ...)' return-value binding on a function returning void",
                      c->bind_name);
        bool found = false;
        for (PostBind *b = bind_head.next; b; b = b->next)
            if (b->name == c->bind_name) {
                found = true;
                break;
            }
        if (found)
            continue;
        PostBind *b = arena_alloc(sizeof(PostBind));
        b->name = c->bind_name;
        b->var = new_var(c->bind_name, fty->return_ty, true);
        b->next = NULL;
        bind_cur = bind_cur->next = b;
    }
    current_fn_postcond_binds = bind_head.next;
    current_fn_postconds = fty->postconds;

    RangeBind *param_env = opt_O3 ? build_range_env(params) : NULL;
    Node head = {0};
    Node *cur = &head;
    for (Contract *c = fty->preconds; c; c = c->next) {
        Node *check = compile_one_contract(c, true, fn_name, tok, param_env);
        if (check)
            cur = cur->next = check;
    }
    return head.next;
}

// Rewrite a `return EXPR;` / `return;` / implicit-fallthrough ND_RETURN
// (already built by the caller, with cleanup_begin/cleanup_end/
// defer_retspill already set from ITS OWN lexical point — untouched here)
// to check every active postcondition first. A named post(NAME: ...)
// binding is assigned EXACTLY ONCE (from the primary — first distinct
// name's — binding), copied to any other distinct binding, and the
// return expression is swapped for a read of the primary binding so the
// checked value, not a freshly re-evaluated one, is what's returned. A
// no-op (returns `node` unchanged) when no postconditions are active.
static Node *apply_postconds_to_return(Node *node, Token *tok) {
    if (!current_fn_postconds)
        return node;
    // Must be captured from the ORIGINAL return expression before the
    // rewrite below swaps node->lhs for a read of the bound temp.
    RangeBind *range_env = NULL;
    if (opt_O3) {
        range_env = build_range_env(current_fn_range_params);
        if (node->lhs && current_fn_postcond_binds) {
            Range ret_range = compute_range(node->lhs, range_env);
            for (PostBind *b = current_fn_postcond_binds; b; b = b->next) {
                RangeBind *rb = arena_alloc(sizeof(RangeBind));
                rb->var = b->var;
                rb->range = ret_range;
                rb->next = range_env;
                range_env = rb;
            }
        }
    }
    Node head = {0};
    Node *cur = &head;
    if (node->lhs && current_fn_postcond_binds) {
        LVar *primary = current_fn_postcond_binds->var;
        Node *assign = new_binary(ND_ASSIGN, new_var_node(primary, tok), node->lhs, tok);
        check_type(assign);
        cur = cur->next = new_unary(ND_EXPR_STMT, assign, tok);
        for (PostBind *b = current_fn_postcond_binds->next; b; b = b->next) {
            Node *cp = new_binary(ND_ASSIGN, new_var_node(b->var, tok), new_var_node(primary, tok), tok);
            check_type(cp);
            cur = cur->next = new_unary(ND_EXPR_STMT, cp, tok);
        }
        node->lhs = new_var_node(primary, tok);
    }
    for (Contract *c = current_fn_postconds; c; c = c->next) {
        Node *check = compile_one_contract(c, false, parser_current_fn, tok, range_env);
        if (check)
            cur = cur->next = check;
    }
    cur->next = node;
    Node *blk = new_node(ND_BLOCK, tok);
    blk->body = head.next;
    return blk;
}

// Does the last top-level statement in `n` (unwrapping a synthetic
// postcondition-check ND_BLOCK from apply_postconds_to_return()) already
// return, so an implicit trailing return synthesized after it would be
// dead code?
static bool ends_in_return(Node *n) {
    if (!n)
        return false;
    if (n->kind == ND_RETURN)
        return true;
    if (n->kind == ND_BLOCK) {
        Node *last = n->body;
        if (!last)
            return false;
        while (last->next)
            last = last->next;
        return ends_in_return(last);
    }
    return false;
}

static Node *make_cleanup_stmt(LVar *var, Token *tok) {
    Node *call = new_node(ND_FUNCALL, tok);
    call->funcname = var->cleanup_func;
    LVar *fn = find_global_name(var->cleanup_func);
    if (fn)
        call->lhs = new_var_node(fn, tok);
    call->args = new_unary(ND_ADDR, new_var_node(var, tok), tok);

    Node *stmt = new_unary(ND_EXPR_STMT, call, tok);
    check_type(stmt);
    return stmt;
}

// Append cleanup stmts to a node list in-place; returns the (possibly new) list head.
static Node *append_cleanup_flat(Node *body, LVar *begin, LVar *end, Token *tok) {
    Node head = {};
    Node *cur = &head;
    head.next = body;
    while (cur->next)
        cur = cur->next;
    for (LVar *var = begin; var && var != end; var = var->next) {
        if (var->is_local && var->defer_stmt)
            cur = cur->next = var->defer_stmt;
        else if (var->is_local && var->cleanup_func)
            cur = cur->next = make_cleanup_stmt(var, tok);
    }
    return head.next;
}

static Node *append_cleanup_range(Node *body, LVar *begin, LVar *end, Token *tok) {
    Node head = {};
    Node *cur = &head;

    if (body) {
        head.next = body;
        while (cur->next)
            cur = cur->next;
    }

    for (LVar *var = begin; var && var != end; var = var->next) {
        if (var->is_local && var->defer_stmt)
            cur = cur->next = var->defer_stmt;
        else if (var->is_local && var->cleanup_func)
            cur = cur->next = make_cleanup_stmt(var, tok);
        if (var->is_local && var->ty->kind == TY_VLA) {
            Node *v = new_node(ND_EXPR_STMT, tok);
            Node *a = new_node(ND_ALLOCA, tok);
            a->kind = ND_ALLOCA_ZINIT;
            a->var = var;
            a->lhs = new_num(0, tok);
            v->lhs = a;
            cur = cur->next = v;
        }
    }

    if (!head.next)
        return body;

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    return node;
}

static Token *skip_balanced(Token *tok) {
    int depth = 0;
    do {
        if (equalc(tok, "("))
            depth++;
        else if (equalc(tok, ")"))
            depth--;
        tok = tok->next;
    } while (depth > 0 && tok->kind != TK_EOF);
    return tok;
}

static Type *type_name(Token **rest, Token *tok);

static Token *read_type_attrs(Token *tok, int *align, VarAttr *attr);
static bool in_type_name = false;
static bool in_compound_literal = false;

static Token *skip_attributes(Token *tok) {
    return read_type_attrs(tok, NULL, NULL);
}

static unsigned char collect_type_quals(Token **rest, Token *tok) {
    unsigned char q = 0;
    while (kw_is(tok, KW_QUAL)) {
        switch (tok->kw) {
        case ID_CONST:
        case ID___CONST:
        case ID___CONST__:
            q |= QUAL_CONST;
            break;
        case ID_VOLATILE:
        case ID___VOLATILE:
        case ID___VOLATILE__:
            q |= QUAL_VOLATILE;
            break;
        case ID_RESTRICT:
        case ID___RESTRICT:
        case ID___RESTRICT__:
            q |= QUAL_RESTRICT;
            break;
        case ID__ATOMIC:
            q |= QUAL_ATOMIC;
            break;
        }
        tok = tok->next;
    }
    *rest = tok;
    return q;
}


static bool is_typename(Token *tok) {
    // C23: [[ starts an attribute list, valid in declaration specifiers
    if (equalc(tok, "[") && tok->next && equalc(tok->next, "[") &&
        tok->ptr + tok->len == tok->next->ptr)
        return true;
    if (tok->kw == ID___ATTRIBUTE || tok->kw == ID___ATTRIBUTE__ ||
#ifdef _WIN32
        tok->kw == ID___DECLSPEC ||
#endif
        tok->kw == ID__ALIGNAS)
        return true;
    // `asm`/`__asm__` can never lead a declaration-specifier list (GNU C's
    // asm-name-specifier only ever trails a declarator, e.g. `int x
    // __asm__("name");`) — it's either a basic/extended inline-asm
    // statement or, appearing here mid-lookahead, garbage. Don't route it
    // through skip_attributes(): read_type_attrs's "simple __asm__(\"label\")
    // for symbol naming" branch can't tell a bare inline-asm statement
    // apart from a real name-specifier (both are just `__asm__("...")`
    // with no operand colons), and firing it here — during a pure,
    // supposedly side-effect-free peek — sets the global pending_asm_name
    // as a side effect. If the peek then (correctly) decides this isn't a
    // declaration, that leftover pending_asm_name silently attaches to
    // whatever declaration/function comes next (e.g. a bare `__asm__("x0,
    // x1, x2 style garbage");` statement as a function's first statement
    // corrupts that function's own linker symbol name).
    if (tok->kw == ID_ASM || tok->kw == ID___ASM || tok->kw == ID___ASM__)
        return false;
    tok = skip_attributes(tok);
    // "thread_local"/"constexpr" are keywords only from C23 onward (C11
    // has no equivalent identifier reservation for either spelling) —
    // don't let a pre-C23 parameter/variable named "thread_local" or
    // "constexpr" be mistaken for the start of a type-name, e.g. inside
    // "(thread_local)" used to disambiguate a cast/compound-literal from
    // a parenthesized expression (kefir's own boolean-parameter check
    // `if (thread_local) ...`). Falls through to the ordinary shadowing
    // checks below, same as any other plain identifier.
    bool is_std_kw = kw_is(tok, KW_TYPE | KW_QUAL | KW_STORAGE);
    if (is_std_kw && (tok->kw == ID_THREAD_LOCAL || tok->kw == ID_CONSTEXPR) &&
        (!opt_std_version || strcmp(opt_std_version, "202311L") < 0))
        is_std_kw = false;
    if (is_std_kw)
        return true;
    // A typedef name is an ordinary identifier (C11 6.2.3): a local
    // variable or parameter of the same name declared in an enclosing
    // scope shadows it, same as it would shadow another variable. Without
    // this check, e.g. `typedef int (*initxattrs)(...); ... int f(const
    // initxattrs initxattrs) { if (initxattrs) ... }` misparses the local
    // use as a type name (linux/security/security.c) instead of the
    // shadowing parameter.
    LVar *shadow = find_var(tok);
    if (shadow && shadow->is_local)
        return false;
    return find_typedef(tok) != NULL;
}

static Type *declspec(Token **rest, Token *tok, VarAttr *attr);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
bool eval_const_expr(Node *node, long long *val);
static bool eval_const_fexpr(Node *node, long double *val);
static bool eval_const_addr_expr(Node *node, long long *val);
static Node *type_size_node(Type *ty, Token *tok);
static void global_initializer_impl(Token **rest, Token *tok, LVar *var);
static void global_initializer(Token **rest, Token *tok, LVar *var);

static void maybe_update_align(int *align, int value) {
    if (align && value > *align)
        *align = value;
}
// C23 6.7.13: an attribute-specifier-sequence immediately before a
// struct/union/enum specifier is only a constraint violation when the
// declaration is genuinely empty (nothing after the specifier but ';') --
// e.g. `[[]] struct s { int a; };` or `[[]] struct s;`. A real member/
// variable declaration with a declarator after the specifier (e.g.
// `[[_counted_by(n)]] struct thread threads[];`) is valid and must not
// be flagged. Scans past the optional tag name, an optional enum fixed
// underlying type, and an optional `{ ... }` body to see what follows.
static bool is_empty_tag_decl(Token *after_attr) {
    Token *t = after_attr->next; // skip struct/union/enum keyword
    if (t && t->kind == TK_IDENT) t = t->next; // optional tag name
    if (t && equalc(t, ":")) { // enum e : int
        t = t->next;
        while (t && t->kind != TK_EOF && !equalc(t, "{") && !equalc(t, ";"))
            t = t->next;
    }
    if (t && equalc(t, "{")) { // optional definition body
        int depth = 1;
        t = t->next;
        while (t && t->kind != TK_EOF && depth > 0) {
            if (equalc(t, "{")) depth++;
            else if (equalc(t, "}"))
                depth--;
            t = t->next;
        }
    }
    return t && equalc(t, ";");
}
static Token *read_type_attrs(Token *tok, int *align, VarAttr *attr) {
    while (true) {

        // _Pragma("string") — C99 pragma operator, treat as no-op
        if (tok->kw == ID__PRAGMA) {
            tok = tok->next;
            tok = skip(tok, "(");
            if (tok->kind == TK_STR)
                tok = tok->next;
            tok = skip(tok, ")");
            continue;
        }
        // C23 [[attribute]] syntax — parse into VarAttr
        if (equalc(tok, "[") && equalc(tok->next, "[") && tok->ptr + tok->len == tok->next->ptr) {
            // In C11 mode, [[ is two separate [ tokens
            if (opt_pedantic && opt_std_version && strcmp(opt_std_version, "202311L") < 0) {
                warn_tok(tok, "[[attributes]] before C23 are not supported");
                // skip to matching ]]
                int bd = 1;
                if (tok->len == 2 && tok->ptr[0] == '[' && tok->ptr[1] == '[')
                    tok = tok->next;
                else
                    tok = tok->next->next;
                while (tok && tok->kind != TK_EOF && bd > 0) {
                    if (equalc(tok, "[") && equalc(tok->next, "[") && tok->ptr + tok->len == tok->next->ptr) bd++;
                    else if (equalc(tok, "]") && equalc(tok->next, "]")) {
                        bd--;
                        tok = tok->next;
                    }
                    tok = tok->next;
                }
                continue;
            }
            // Skip past the [[ tokens (may be one or two tokens)
            if (tok->len == 2 && tok->ptr[0] == '[' && tok->ptr[1] == '[')
                tok = tok->next; // single [[ token
            else
                tok = tok->next->next; // two separate [ tokens
            // Empty [[]]: check what follows to decide if it's an empty declaration

            bool empty_attr = equalc(tok, "]") && tok->next && equalc(tok->next, "]") &&
                tok->ptr + tok->len == tok->next->ptr;
            Token *after_attr = empty_attr ? tok->next->next : NULL;
            while (tok && tok->kind != TK_EOF &&
                   !(equalc(tok, "]") && tok->next && equalc(tok->next, "]") && tok->ptr + tok->len == tok->next->ptr)) {
                if (equalc(tok, ",")) {
                    tok = tok->next;
                    continue;
                }
                // namespace:: (two adjacent : tokens)
                if (tok->next && equalc(tok->next, ":")) {
                    Token *c1 = tok->next;
                    if (c1->next && equalc(c1->next, ":") &&
                        c1->ptr + c1->len == c1->next->ptr) {
                        // gnu::packed / __gnu__::packed alias the legacy
                        // __attribute__((packed)) — same packing effect.
                        bool is_gnu_ns = equalc(tok, "gnu") || equalc(tok, "__gnu__");
                        Token *ns_name = c1->next->next;
                        if (is_gnu_ns && ns_name &&
                            (equalc(ns_name, "packed") || equalc(ns_name, "__packed__"))) {
                            if (attr)
                                attr->is_packed = true;
                            maybe_update_align(align, 1);
                            tok = ns_name->next;
                            if (equalc(tok, "("))
                                tok = skip_balanced(tok);
                            continue;
                        }
                        tok = c1->next->next; // skip ident ::
                        continue;
                    }
                    // Single colon after namespace name: expected ']' before ':'
                    error_tok(c1, "expected ']' before ':'");
                }
                // __extension__ allows GNU attrs in standard [[]] syntax

                bool consumed = false;
                if (tok->kind != TK_IDENT)
                    error_tok(tok, "expected attribute identifier");
                if (attr) {
                    if (equalc(tok, "noreturn"))
                        attr->is_noreturn = true;
                    else if (equalc(tok, "deprecated")) {
                        attr->is_deprecated = true;
                        Token *next = tok->next;
                        if (equalc(next, "(")) {
                            // C23 6.7.13.4: one optional string literal
                            if (equalc(next->next, ")"))
                                error_tok(next, "parentheses must be omitted if "
                                                "attribute argument list is empty");
                            if (!next->next || next->next->kind != TK_STR)
                                error_tok(next->next ? next->next : next,
                                          "expected string literal in deprecated "
                                          "attribute");
                            size_t len = next->next->len;
                            attr->deprecated_msg = arena_alloc(len + 3);
                            attr->deprecated_msg[0] = '"';
                            memcpy(attr->deprecated_msg + 1, next->next->str, len);
                            attr->deprecated_msg[len + 1] = '"';
                            attr->deprecated_msg[len + 2] = '\0';
                            tok = next->next;
                            tok = skip(tok->next, ")");
                            consumed = true;
                        }
                    } else if (equalc(tok, "reproducible"))
                        attr->is_reproducible = true;
                    else if (equalc(tok, "unsequenced"))
                        attr->is_unsequenced = true;
                }
                if (!consumed) tok = tok->next;
                if (equalc(tok, "(")) {
                    int pdepth = 1;
                    tok = tok->next;
                    while (pdepth > 0 && tok && tok->kind != TK_EOF) {
                        if (equalc(tok, "(")) pdepth++;
                        else if (equalc(tok, ")"))
                            pdepth--;
                        tok = tok->next;
                    }
                }
            }
            if (tok) tok = tok->next->next; // skip ]]
            // Empty [[]] before struct/union/enum is only invalid in C23+,
            // and only when the declaration truly has no declarator.
            if (empty_attr && after_attr && opt_std_version &&
                strcmp(opt_std_version, "202311L") >= 0 &&
                (equalc(after_attr, "struct") || equalc(after_attr, "union") ||
                 equalc(after_attr, "enum")) &&
                is_empty_tag_decl(after_attr))
                error_tok(after_attr, "empty declaration");
            continue;
        }
        if (tok->kw == ID__ALIGNAS ||
            (tok->kw == ID_ALIGNAS && opt_std_version && strcmp(opt_std_version, "202311L") >= 0)) {
            // C11 6.7.5: alignment specifiers are not part of a type name,
            // except in compound literals (C23 6.5.2.5).
            if (in_type_name && !in_compound_literal)
                error_tok(tok, "alignment specified for type name");
            tok = tok->next;
            tok = skip(tok, "(");
            if (attr) attr->has_alignas = true;
            if (is_typename(tok)) {
                Token *aty_tok = tok;
                Type *ty = type_name(&tok, tok);
                if (ty->kind == TY_FUNC)
                    error_tok(aty_tok, "alignment specified for function type");
                if (ty->kind == TY_VOID)
                    error_tok(aty_tok, "alignment specified for void type");
                if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) &&
                    !ty->has_body)
                    error_tok(aty_tok, "alignment specified for incomplete type");
                maybe_update_align(align, ty->align);
            } else {
                Node *node = expr(&tok, tok);
                check_type(node);
                long long val = 0;
                if (node->ty &&
                    (node->ty->kind == TY_NULLPTR_T || !is_integer(node->ty)))
                    error_tok(tok, "alignment is not an integer constant");
                if (!eval_const_expr(node, &val))
                    error_tok(tok, "expected alignment");
                // C11 6.7.5: valid alignments are 0 or a positive power of 2
                if (val < 0 || (val & (val - 1)))
                    error_tok(node->tok, "alignment is not a positive power of 2");
                maybe_update_align(align, (int)val);
            }
            tok = skip(tok, ")");
            continue;
        }


        if (tok->kw == ID_ASM || tok->kw == ID___ASM || tok->kw == ID___ASM__) {
            Token *start = tok;
            tok = tok->next;
            // Before consuming "(", check if next is "(" — if not, it's not an asm attribute
            if (!equalc(tok, "(")) {
                // Could be __asm__ statement without parens (but we don't handle that here)
                tok = start;
                return tok;
            }
            tok = skip(tok, "(");
            char asm_buf[256];
            asm_buf[0] = '\0';
            int asm_len = 0;
            while (tok->kind == TK_STR || (tok->kind == TK_IDENT && equalc(tok, "_"))) {
                if (tok->kind == TK_STR && asm_len + tok->len < (int)sizeof(asm_buf) - 1) {
                    memcpy(asm_buf + asm_len, tok->str, tok->len);
                    asm_len += tok->len;
                    asm_buf[asm_len] = '\0';
                }
                tok = tok->next;
            }
            // Simple __asm__("label") for symbol naming — no operands
            if (equalc(tok, ")")) {
                tok = skip(tok, ")");
                if (asm_len > 0)
                    pending_asm_name = str_intern(asm_buf, asm_len);
                continue;
            }
            // Inline asm with operand sections — don't consume, let stmt() handle it
            tok = start;
            return tok;
        }

        if (tok->kw == ID___ATTRIBUTE || tok->kw == ID___ATTRIBUTE__) {
            tok = tok->next;
            tok = skip(tok, "(");
            tok = skip(tok, "(");
            while (!(equalc(tok, ")") && equalc(tok->next, ")"))) {
                if (equalc(tok, "__cleanup__") || equalc(tok, "cleanup")) {
                    pending_cleanup_tok = tok;
                    tok = tok->next;
                    tok = skip(tok, "(");
                    if (tok->kind == TK_IDENT)
                        pending_cleanup_func = tok->name;
                    tok = tok->next;
                    tok = skip(tok, ")");
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "aligned") || equalc(tok, "__aligned__")) {
                    tok = tok->next;
                    if (equalc(tok, "(")) {
                        tok = tok->next;
                        if (is_typename(tok)) {
                            Type *ty = type_name(&tok, tok);
                            maybe_update_align(align, ty->align);
                        } else {
                            Node *node = expr(&tok, tok);
                            long long val = 0;
                            if (!eval_const_expr(node, &val))
                                error_tok(tok, "expected alignment");
                            maybe_update_align(align, (int)val);
                        }
                        tok = skip(tok, ")");
                    }
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "weak") || equalc(tok, "__weak__")) {
                    if (attr)
                        attr->is_weak = true;
                    tok = tok->next;
                    if (equalc(tok, "("))
                        tok = skip_balanced(tok);
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }
                // __attribute__((visibility("hidden|default|internal|protected")))
                // — glib's G_GNUC_INTERNAL and _GLIB_EXTERN rely on it, and
                // rcc would otherwise export every internal symbol (its
                // check-abis.sh flags exactly those leaks).
                if (equalc(tok, "visibility") || equalc(tok, "__visibility__")) {
                    tok = tok->next;
                    tok = skip(tok, "(");
                    if (attr && (tok->kind == TK_STR || tok->kind == TK_IDENT)) {
                        const char *val = tok->kind == TK_STR ? tok->str : tok->name;
                        int vlen = tok->len;
                        if (tok->kind == TK_STR && vlen >= 2 &&
                            (tok->str[0] == '"' || tok->str[0] == '\'')) {
                            val = tok->str + 1;
                            vlen -= 2;
                        }
                        attr->has_visibility = true;
                        if ((vlen == 6 && !memcmp(val, "hidden", 6)) ||
                            (vlen == 8 && !memcmp(val, "internal", 8)))
                            attr->visibility = STV_HIDDEN;
                        else if (vlen == 9 && !memcmp(val, "protected", 9))
                            attr->visibility = STV_PROTECTED;
                        else
                            attr->visibility = STV_DEFAULT; // "default" (or unknown)
                    }
                    tok = tok->next;
                    tok = skip(tok, ")");
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }
                // __common__: GCC attribute for tentative definitions to emit
                // as COMMON symbols. rcc emits weak (STB_WEAK) which achieves
                // the same linker-level merging behavior. Found via redis's
                // redismodule.h which uses REDISMODULE_ATTR_COMMON = __attribute__((__common__))
                // on function pointer globals defined in multiple TUs.
                if (equalc(tok, "common") || equalc(tok, "__common__")) {
                    if (attr)
                        attr->is_weak = true;
                    tok = tok->next;
                    if (equalc(tok, "("))
                        tok = skip_balanced(tok);
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "used") || equalc(tok, "__used__")) {
                    // Forces emission even when otherwise provably dead —
                    // most relevantly here, exempts a `static inline`
                    // function from the omit-if-never-referenced rule (see
                    // eliminate_unused_static_inline() in opt.c): real GCC
                    // still emits a `static inline __attribute__((used))`
                    // function's body even though nothing in the TU calls
                    // it (confirmed against real gcc -O0/-O2 output).
                    if (attr)
                        attr->is_used = true;
                    tok = tok->next;
                    if (equalc(tok, "("))
                        tok = skip_balanced(tok);
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "always_inline") || equalc(tok, "__always_inline__")) {
                    if (attr)
                        attr->is_always_inline = true;
                    tok = tok->next;
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "gnu_inline") || equalc(tok, "__gnu_inline__")) {
                    if (attr)
                        attr->is_gnu_inline = true;
                    tok = tok->next;
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "ms_struct")) {
                    if (attr)
                        attr->bitfield_mode = BF_MODE_MS;
                    tok = tok->next;
                    if (equalc(tok, "("))
                        tok = skip_balanced(tok);
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "gcc_struct")) {
                    if (attr)
                        attr->bitfield_mode = BF_MODE_GCC;
                    tok = tok->next;
                    if (equalc(tok, "("))
                        tok = skip_balanced(tok);
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "packed") || equalc(tok, "__packed__")) {
                    if (attr)
                        attr->is_packed = true;
                    // Also set align=1 so member-level packed attr takes effect
                    maybe_update_align(align, 1);
                    tok = tok->next;
                    if (equalc(tok, "("))
                        tok = skip_balanced(tok);
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "transparent_union") || equalc(tok, "__transparent_union__")) {
                    if (attr)
                        attr->is_transparent_union = true;
                    tok = tok->next;
                    if (equalc(tok, "("))
                        tok = skip_balanced(tok);
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "constructor") || equalc(tok, "__constructor__")) {
                    pending_constructor = true;
                    tok = tok->next;
                    if (equalc(tok, "("))
                        tok = skip_balanced(tok);
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "destructor") || equalc(tok, "__destructor__")) {
                    pending_destructor = true;
                    tok = tok->next;
                    if (equalc(tok, "("))
                        tok = skip_balanced(tok);
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "alias") || equalc(tok, "__alias__")) {
                    tok = tok->next;
                    tok = skip(tok, "(");
                    if (tok->kind == TK_STR) {
                        int len = tok->len;
                        if (len >= 2 && (tok->str[0] == '"' || tok->str[0] == '\''))
                            len -= 2;
                        char *target = malloc(len + 1);
                        if (len >= 2 && (tok->str[0] == '"' || tok->str[0] == '\''))
                            memcpy(target, tok->str + 1, len);
                        else
                            memcpy(target, tok->str, len + 1);
                        target[len] = '\0';
                        pending_alias_target = str_intern(target, len);
                        free(target);
                    }
                    tok = tok->next;
                    tok = skip(tok, ")");
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "section") || equalc(tok, "__section__")) {
                    tok = tok->next;
                    tok = skip(tok, "(");
                    if (tok->kind == TK_STR) {
                        int len = tok->len;
                        if (len >= 2 && (tok->str[0] == '"' || tok->str[0] == '\''))
                            len -= 2;
                        char *name = malloc(len + 1);
                        if (len >= 2 && (tok->str[0] == '"' || tok->str[0] == '\''))
                            memcpy(name, tok->str + 1, len);
                        else
                            memcpy(name, tok->str, len + 1);
                        name[len] = '\0';
                        pending_section_name = str_intern(name, len);
                        free(name);
                    }
                    tok = tok->next;
                    tok = skip(tok, ")");
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "target_clones")) {
                    tok = tok->next;
                    tok = skip(tok, "(");
                    // Parse comma-separated string list: "default","avx2",...
                    int cap = 4;
                    pending_target_clones = calloc((size_t)cap, sizeof(char *));
                    pending_target_clones_n = 0;
                    while (tok->kind == TK_STR) {
                        char *s = tok->str;
                        int len = tok->len;
                        // strip quotes
                        if (len >= 2 && (s[0] == '"' || s[0] == '\'')) {
                            s++;
                            len -= 2;
                        }
                        if (pending_target_clones_n + 1 >= cap) {
                            cap *= 2;
                            char **ntc = realloc(pending_target_clones, (size_t)cap * sizeof(char *));
                            if (!ntc) {
                                error("out of memory growing target_clones list");
                            }
                            pending_target_clones = ntc;
                        }
                        pending_target_clones[pending_target_clones_n++] = str_intern(s, len);
                        tok = tok->next;
                        if (equalc(tok, ","))
                            tok = tok->next;
                    }
                    pending_target_clones[pending_target_clones_n] = NULL; // terminator
                    tok = skip(tok, ")");
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "target")) {
                    tok = tok->next;
                    if (equalc(tok, "(")) {
                        tok = tok->next;
                        if (tok->kind == TK_STR) {
                            char *s = tok->str;
                            int len = tok->len;
                            if (len >= 2 && (s[0] == '"' || s[0] == '\'')) {
                                s++;
                                len -= 2;
                            }
                            pending_target_attr = str_intern(s, len);
                        }
                        tok = tok->next;
                        tok = skip(tok, ")");
                    }
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "warning") || equalc(tok, "__warning__")) {
                    tok = tok->next;
                    tok = skip(tok, "(");
                    char *msg = NULL;
                    if (tok->kind == TK_STR)
                        msg = tok->str;
                    tok = tok->next;
                    tok = skip(tok, ")");
                    if (attr && msg)
                        attr->diag_warning = str_intern(msg, strlen(msg));
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "error") || equalc(tok, "__error__")) {
                    tok = tok->next;
                    tok = skip(tok, "(");
                    char *msg = NULL;
                    if (tok->kind == TK_STR)
                        msg = tok->str;
                    tok = tok->next;
                    tok = skip(tok, ")");
                    if (attr && msg)
                        attr->diag_error = str_intern(msg, strlen(msg));
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "diagnose_if")) {
                    tok = tok->next;
                    tok = skip(tok, "(");
                    // Skip the condition expression — not evaluated at declaration time.
                    // clang evaluates at call sites; for now we store the attribute
                    // message and emit it unconditionally at every call site.
                    int depth = 0;
                    while (tok->kind != TK_EOF && !(depth == 0 && equalc(tok, ","))) {
                        if (equalc(tok, "(")) depth++;
                        else if (equalc(tok, ")"))
                            depth--;
                        tok = tok->next;
                    }
                    tok = skip(tok, ",");
                    char *msg = NULL;
                    if (tok->kind == TK_STR)
                        msg = tok->str;
                    tok = tok->next;
                    tok = skip(tok, ",");
                    bool is_error = false;
                    if (tok->kind == TK_STR && tok->str && strcmp(tok->str, "error") == 0)
                        is_error = true;
                    tok = tok->next;
                    tok = skip(tok, ")");
                    if (attr && msg) {
                        DiagEntry *de = arena_alloc(sizeof(DiagEntry));
                        de->msg = str_intern(msg, strlen(msg));
                        de->is_error = is_error;
                        de->next = attr->diag_entries;
                        attr->diag_entries = de;
                    }
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "mode") || equalc(tok, "__mode__")) {
                    tok = tok->next;
                    tok = skip(tok, "(");
                    if (tok->kind == TK_IDENT) {
                        if (equalc(tok, "QI")) pending_mode = 1;
                        else if (equalc(tok, "HI"))
                            pending_mode = 2;
                        else if (equalc(tok, "SI"))
                            pending_mode = 3;
                        else if (equalc(tok, "DI"))
                            pending_mode = 4;
                        else if (equalc(tok, "TI"))
                            pending_mode = 5;
                        tok = tok->next;
                    }
                    tok = skip(tok, ")");
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }
                if (equalc(tok, "pure") || equalc(tok, "__pure__")) {
                    if (attr)
                        attr->is_reproducible = true;
                    tok = tok->next;
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "const") || equalc(tok, "__const__")) {
                    if (attr)
                        attr->is_unsequenced = true;
                    tok = tok->next;
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "vector_size") || equalc(tok, "__vector_size__")) {
                    tok = tok->next;
                    tok = skip(tok, "(");
                    // Clear any in-flight pending size while evaluating the
                    // argument: a nested sizeof(type) re-enters declarator,
                    // which would otherwise consume the pending value and
                    // vectorize the sizeof operand (e.g. attribute text parsed
                    // twice makes vector_size(4*sizeof(float)) yield 64).
                    int saved_pending = pending_vector_size;
                    pending_vector_size = 0;
                    (void)saved_pending;
                    Node *node = expr(&tok, tok);
                    long long val = 0;
                    if (!eval_const_expr(node, &val))
                        error_tok(tok, "expected constant vector_size");
                    pending_vector_size = (int)val;
                    tok = skip(tok, ")");
                    if (equalc(tok, ","))
                        tok = tok->next;
                    continue;
                }

                if (equalc(tok, "(")) {
                    tok = skip_balanced(tok);
                } else {
                    tok = tok->next;
                }

                if (equalc(tok, ","))
                    tok = tok->next;
            }
            tok = skip(tok, ")");
            tok = skip(tok, ")");
            continue;
        }

#ifdef _WIN32
        if (tok->kw == ID___DECLSPEC) {
            tok = tok->next;
            if (equalc(tok, "("))
                tok = skip_balanced(tok);
            continue;
        }
#endif

        break;
    }

    return tok;
}

static bool eval_const_addr_expr(Node *node, long long *val) {
    if (!node) return false;
    switch (node->kind) {
    case ND_MEMBER: {
        long long base_val;
        if (!eval_const_addr_expr(node->lhs, &base_val))
            return false;
        *val = base_val + node->member->offset;
        return true;
    }
    case ND_DEREF:
        return eval_const_expr(node->lhs, val);
    default:
        return eval_const_expr(node, val);
    }
}

// Fold `node` to a compile-time-constant integer if possible; returns
// false (val untouched) for anything not statically foldable (e.g. a
// runtime variable read). Used throughout for array sizes, enum values,
// case labels, static-storage initializers, and _Static_assert.
// Float-aware sibling of eval_const_expr(): evaluates a compile-time
// constant subexpression whose OWN type is a floating type, keeping
// every intermediate result in floating point. eval_const_expr()
// delegates to this whenever it meets a floating-typed node — most
// commonly the operand of an integer cast, e.g. `(size_t)(0.7f *
// 256.0f)`. Without this, eval_const_expr()'s ND_FNUM case truncated
// each float literal to an integer *before* combining them (0.7f -> 0,
// 256.0f -> 256), so `0.7f * 256.0f` folded to 0 instead of 179 --
// found via flatcc's refmap load-factor constant, which made
// `_flatcc_refmap_above_load_factor()`'s `n` compile-time constant
// always 0, so its resize loop's exit condition could never be met and
// `buckets` doubled forever (wrapping to 0 and spinning) the first time
// the hash table actually needed to grow.
static bool eval_const_fexpr(Node *node, long double *val) {
    if (!node)
        return false;
    // Decimal subexpressions are not float-const-foldable: BID bits aren't
    // long doubles. Return false so the runtime __bid_* path is used.
    if (node->ty && is_decimal(node->ty))
        return false;
    if (node->lhs && node->lhs->ty && is_decimal(node->lhs->ty))
        return false;
    if (node->rhs && node->rhs->ty && is_decimal(node->rhs->ty))
        return false;
    long double lhs, rhs;
    switch (node->kind) {
    case ND_FNUM:
        *val = node->fval;
        return true;
    case ND_NUM:
        *val = (long double)node->val;
        return true;
    case ND_ADD:
        return eval_const_fexpr(node->lhs, &lhs) && eval_const_fexpr(node->rhs, &rhs) && ((*val = lhs + rhs), true);
    case ND_SUB:
        return eval_const_fexpr(node->lhs, &lhs) && eval_const_fexpr(node->rhs, &rhs) && ((*val = lhs - rhs), true);
    case ND_MUL:
        return eval_const_fexpr(node->lhs, &lhs) && eval_const_fexpr(node->rhs, &rhs) && ((*val = lhs * rhs), true);
    case ND_DIV:
        if (!eval_const_fexpr(node->lhs, &lhs) || !eval_const_fexpr(node->rhs, &rhs) || rhs == 0)
            return false;
        *val = lhs / rhs;
        return true;
    case ND_NEG:
        return eval_const_fexpr(node->lhs, &lhs) && ((*val = -lhs), true);
    case ND_CAST:
        if (node->lhs && node->lhs->ty && is_flonum(node->lhs->ty))
            return eval_const_fexpr(node->lhs, val);
        {
            // Integer (or other non-float) operand cast to a float type:
            // evaluate the integer side and convert, respecting its
            // signedness so e.g. `(float)(unsigned)-1` doesn't sign-extend.
            long long iv;
            if (!eval_const_expr(node->lhs, &iv))
                return false;
            *val = (node->lhs->ty && node->lhs->ty->is_unsigned) ? (long double)(unsigned long long)iv : (long double)iv;
            return true;
        }
    case ND_COMMA: {
        long long discard;
        return eval_const_expr(node->lhs, &discard) && eval_const_fexpr(node->rhs, val);
    }
    case ND_COND: {
        long long c;
        return eval_const_expr(node->cond, &c) && eval_const_fexpr(c ? node->then : node->els, val);
    }
    default:
        return false;
    }
}

// Reinterpret `v` (already the properly sign/zero-extended 64-bit value of
// an operand whose own declared width is `width_bytes`) as the unsigned
// value it represents once truncated to the common comparison width -
// i.e. C's usual-arithmetic-conversion "convert the signed operand to
// unsigned" step, at the CORRECT bit width rather than raw 64-bit
// reinterpretation. Needed for constant-folding an unsigned comparison
// between two differently-signed narrower-than-64-bit operands: e.g.
// (unsigned)0x80000000u == (int)0x80000000 must be true (both represent
// the 32-bit pattern 0x80000000), but plain `(unsigned long long)lhs ==
// (unsigned long long)rhs` gets 0x80000000 vs 0xFFFFFFFF80000000 (the
// `int` operand's value, -2147483648, sign-extended to 64 bits) - two
// different 64-bit patterns - unless first re-truncated to the 32-bit
// width the comparison actually happens at.
static unsigned long long uval_at_width(long long v, int width_bytes) {
    if (width_bytes <= 0 || width_bytes >= 8)
        return (unsigned long long)v;
    return (unsigned long long)v & ((1ULL << (width_bytes * 8)) - 1);
}

// C11 Annex J.2 / 6.5p5: signed integer overflow is undefined behavior.
// GCC's constant folder warns "integer overflow in expression ...
// results in undefined behavior" (-Woverflow, on by default) whenever
// folding +, -, or * produces a value outside the expression's own
// signed integer type's range (e.g. `__INT_MAX__ * 2`). Unsigned
// overflow is well-defined wraparound, so it's exempt. `raw` is the
// exact mathematical result in a 64-bit accumulator; `raw_overflowed`
// additionally reports whether computing it already overflowed 64 bits
// (relevant only for `long`/`long long`-typed operands).
// Set around purely-speculative eval_const_expr() probes (e.g.
// is_null_pointer_constant() in type.c, called on BOTH arms of every
// conditional expression regardless of which arm a constant condition
// would actually select -- GCC never diagnoses overflow in an
// expression whose value is never required, see PR c/93241's `0 ? (x)
// (INT_MAX + 1) : 1`) so the same fold logic used for real, required
// constant-expression evaluation (static_assert, initializers, case
// labels, ...) doesn't warn as a side effect of an unrelated check.
bool suppress_const_overflow_warn;
static void warn_const_int_overflow(Node *node, long long raw, bool raw_overflowed) {
    if (node->overflow_warned || suppress_const_overflow_warn)
        return;
    Type *ty = node->ty;
    if (!ty || !is_integer(ty) || ty->is_unsigned || ty->kind == TY_BITINT || ty->kind == TY_BOOL)
        return;
    int sz = ty->size;
    if (sz <= 0 || sz > 8)
        return;
    bool overflowed = raw_overflowed;
    if (!overflowed && sz < 8) {
        int bits = sz * 8;
        long long lo = -(1LL << (bits - 1));
        long long hi = (1LL << (bits - 1)) - 1;
        overflowed = raw < lo || raw > hi;
    }
    if (overflowed) {
        node->overflow_warned = true;
        warn_tok(node->tok, "integer overflow in expression");
    }
}

static bool eval_const_expr_impl(Node *node, long long *val) {
    long long lhs;
    long long rhs;

    if (!node)
        return false;

    // A floating-typed subexpression (e.g. the operand of an integer
    // cast) must be folded entirely in floating point and converted to
    // an integer only here, at the boundary -- see eval_const_fexpr().
    if (node->ty && is_flonum(node->ty)) {
        long double fv;
        if (!eval_const_fexpr(node, &fv))
            return false;
        *val = (long long)fv;
        return true;
    }

    // Decimal-typed nodes must NEVER fold as integers: the BID bit pattern
    // is not a plain integer value (non-canonical encodings, decimal
    // arithmetic semantics). Route to runtime __bid_* calls instead. This
    // also covers decimal subexpressions (arithmetic on decimals).
    if (node->ty && is_decimal(node->ty))
        return false;
    if (node->lhs && node->lhs->ty && is_decimal(node->lhs->ty))
        return false;
    if (node->rhs && node->rhs->ty && is_decimal(node->rhs->ty))
        return false;

    // codeql[cpp/long-switch]: central AST-node-kind dispatch; splitting cases into helpers is a large, purely-cosmetic refactor of core compiler internals, not attempted here.
    switch (node->kind) {

    case ND_NUM:
        *val = node->val;
        return true;
    case ND_FNUM:
        *val = (long long)node->fval;
        return true;
    case ND_ADD: {
        if (!eval_const_expr(node->lhs, &lhs) || !eval_const_expr(node->rhs, &rhs))
            return false;
        long long sum;
        bool ovf = __builtin_add_overflow(lhs, rhs, &sum);
        warn_const_int_overflow(node, sum, ovf);
        *val = sum;
        return true;
    }
    case ND_SUB: {
        if (!eval_const_expr(node->lhs, &lhs) || !eval_const_expr(node->rhs, &rhs))
            return false;
        long long diff;
        bool ovf = __builtin_sub_overflow(lhs, rhs, &diff);
        warn_const_int_overflow(node, diff, ovf);
        *val = diff;
        return true;
    }
    case ND_MUL: {
        if (!eval_const_expr(node->lhs, &lhs) || !eval_const_expr(node->rhs, &rhs))
            return false;
        long long prod;
        bool ovf = __builtin_mul_overflow(lhs, rhs, &prod);
        warn_const_int_overflow(node, prod, ovf);
        *val = prod;
        return true;
    }
    case ND_DIV:
        if (!eval_const_expr(node->lhs, &lhs) || !eval_const_expr(node->rhs, &rhs) || rhs == 0)
            return false;
        if (node->lhs->ty && node->lhs->ty->is_unsigned) {
            unsigned long long ulhs = (unsigned long long)lhs;
            unsigned long long urhs = (unsigned long long)rhs;
            *val = (long long)(ulhs / urhs);
        } else {
            *val = rhs == -1 ? -lhs : lhs / rhs;
        }
        return true;
    case ND_MOD:
        if (!eval_const_expr(node->lhs, &lhs) || !eval_const_expr(node->rhs, &rhs) || rhs == 0)
            return false;
        if (node->lhs->ty && node->lhs->ty->is_unsigned) {
            unsigned long long ulhs = (unsigned long long)lhs;
            unsigned long long urhs = (unsigned long long)rhs;
            *val = (long long)(ulhs % urhs);
        } else {
            *val = rhs == -1 ? 0 : lhs % rhs;
        }
        return true;
    case ND_SHL:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs << rhs), true);
    case ND_SHR:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = node->lhs->ty && node->lhs->ty->is_unsigned ? (long long)((unsigned long long)lhs >> rhs) : lhs >> rhs), true);
    case ND_BITAND:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs & rhs), true);
    case ND_BITXOR:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs ^ rhs), true);
    case ND_BITOR:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs | rhs), true);
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE: {
        if ((node->lhs->ty && is_decimal(node->lhs->ty)) ||
            (node->rhs->ty && is_decimal(node->rhs->ty)))
            return false;
        // A flonum operand must be compared in genuine floating point,
        // not silently truncated to `long long` first (an out-of-range
        // value like INFINITY, or any float too large for long long's
        // range, is UB to convert and would misjudge the comparison
        // entirely) -- found via GCC torture's c23-float-3.c, `INFINITY
        // > FLT_MAX`.
        if ((node->lhs->ty && is_flonum(node->lhs->ty)) ||
            (node->rhs->ty && is_flonum(node->rhs->ty))) {
            long double flhs, frhs;
            if (!eval_const_fexpr(node->lhs, &flhs) || !eval_const_fexpr(node->rhs, &frhs))
                return false;
            // codeql[cpp/equality-on-floats]: implements C's own ==/!=
            // for float constant-folding -- must be bit-exact IEEE754
            // comparison to match runtime evaluation, not fuzzy/epsilon.
            if (node->kind == ND_EQ) *val = flhs == frhs;
            else if (node->kind == ND_NE)
                *val = flhs != frhs;
            else if (node->kind == ND_LT)
                *val = flhs < frhs;
            else
                *val = flhs <= frhs;
            return true;
        }
        if (!eval_const_expr(node->lhs, &lhs) || !eval_const_expr(node->rhs, &rhs))
            return false;
        bool uns = (node->lhs->ty && node->lhs->ty->is_unsigned) ||
            (node->rhs->ty && node->rhs->ty->is_unsigned);
        if (uns) {
            // Usual arithmetic conversions compare at the WIDER operand's
            // width, not raw 64-bit - a narrower signed value's sign-
            // extended 64-bit pattern must be re-truncated to that common
            // width before reinterpreting as unsigned, or e.g. (int)
            // 0x80000000 (== -2147483648, sign-extended to
            // 0xFFFFFFFF80000000 in `rhs`) never compares equal to the
            // unsigned 32-bit value 0x80000000 it's bit-identical to.
            int lw = (node->lhs->ty && node->lhs->ty->size > 0) ? (int)node->lhs->ty->size : 8;
            int rw = (node->rhs->ty && node->rhs->ty->size > 0) ? (int)node->rhs->ty->size : 8;
            int w = lw > rw ? lw : rw;
            unsigned long long ul = uval_at_width(lhs, w);
            unsigned long long ur = uval_at_width(rhs, w);
            if (node->kind == ND_EQ) *val = ul == ur;
            else if (node->kind == ND_NE)
                *val = ul != ur;
            else if (node->kind == ND_LT)
                *val = ul < ur;
            else
                *val = ul <= ur;
        } else {
            if (node->kind == ND_EQ) *val = lhs == rhs;
            else if (node->kind == ND_NE)
                *val = lhs != rhs;
            else if (node->kind == ND_LT)
                *val = lhs < rhs;
            else
                *val = lhs <= rhs;
        }
        return true;
    }
    case ND_LOGAND:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs && rhs), true);
    case ND_LOGOR:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs || rhs), true);
    case ND_NEG: {
        if (!eval_const_expr(node->lhs, &lhs)) return false;
        // C11 6.3.1.1p2 usual integer promotions: an unsigned operand
        // narrower than int (char/short/bool, and any bitfield <32 bits)
        // promotes to plain (signed) int before the negation -- the
        // "else" branch's plain `-lhs` already gives that answer, since
        // lhs already holds the correctly-sign-extended numeric value.
        // Only an operand whose promoted type is STILL unsigned (rank >=
        // int, e.g. unsigned int/uint32_t, unsigned long) negates with
        // unsigned wraparound -- and that wraparound must happen at the
        // operand's OWN width, not always 64 bits: negating a masked
        // 32-bit value (e.g. `-(uint32_t)-2` == `-(uint32_t)0xfffffffe`)
        // as a 64-bit unsigned quantity produced a huge 64-bit result
        // instead of wrapping back to 2 within 32 bits, e.g. PHP's
        // `ZendAccelerator.h`'s HT_MIN_MASK-derived array dimension
        // `uint32_t uninitialized_bucket[-HT_MIN_MASK]` (HT_MIN_MASK is
        // `(uint32_t) -2`) was rejected as "negative array size".
        if (node->lhs->ty && node->lhs->ty->is_unsigned && node->lhs->ty->size >= 4) {
            unsigned long long r = -(unsigned long long)lhs;
            int sz = node->lhs->ty->size;
            if (sz < 8)
                r &= (1ULL << (sz * 8)) - 1;
            *val = (long long)r;
        } else {
            *val = -lhs;
        }
        return true;
    }
    case ND_NOT:
        return eval_const_expr(node->lhs, &lhs) && ((*val = !lhs), true);
    case ND_BITNOT:
        return eval_const_expr(node->lhs, &lhs) && ((*val = ~lhs), true);
    case ND_CAST: {
        // C11 6.3.1.2: converting any scalar to _Bool yields 0 if the
        // value compares equal to 0, 1 otherwise -- not a truncating
        // bit-copy (matches gen_cast_reg's runtime codegen). Must be
        // checked before the generic eval_const_expr(node->lhs, ...)
        // below: for a flonum operand (e.g. (bool)0.5), that call
        // truncates toward zero first (0.5 -> 0), which would already
        // have destroyed the fractional part this truthiness check
        // needs.
        if (node->ty && node->ty->kind == TY_BOOL) {
            if (node->lhs->ty && is_flonum(node->lhs->ty)) {
                long double fv;
                if (!eval_const_fexpr(node->lhs, &fv))
                    return false;
                *val = fv != 0.0L;
                return true;
            }
            if (!eval_const_expr(node->lhs, val))
                return false;
            *val = *val != 0;
            return true;
        }
        if (!eval_const_expr(node->lhs, val))
            return false;
        if (!node->ty || !is_integer(node->ty))
            return true;
        int sz = node->ty->size;
        if (sz <= 0 || sz >= 8)
            return true;
        int bits = sz * 8;
        // sz is in (0,8) here (line 1964 already returned for sz<=0||sz>=8),
        // so bits=sz*8 is in [8,56] and 1ULL<<bits never overflows.
        unsigned long long mask = (1ULL << bits) - 1;
        if (node->ty->is_unsigned) {
            *val &= mask;
        } else {
            *val &= mask;
            if (*val & (1ULL << (bits - 1)))
                *val |= ~mask;
        }
        return true;
    }
    case ND_SIZEOF: {
        // A VLA's dimension expression fails the strict C integer-
        // constant-expression test whenever it involves string-literal
        // indexing (6.6p6 explicitly excludes string literals from an
        // ICE), even when every value along the way is genuinely
        // resolvable at compile time -- the classic GNU
        // `sizeof(char[1-2*COND])` negative-array-size static-assert
        // trick (e.g. ruby's `rb_scan_args_verify`, walking a format
        // string literal through nested-ternary macros). Real GCC/Clang
        // only resolve this via their optimizer (constant propagation +
        // dead-code elimination at -O2, matching the fact that this
        // fails identically at -O0). rcc has no such optimizer, but the
        // type itself is intentionally left as a genuine VLA here
        // (matching the frontend's own classification, so runtime
        // VLA/`alloca` behavior for a truly-variable dimension is
        // unaffected) -- only THIS constant-expression fold is lenient:
        // recompute the VLA's runtime size expression and try folding
        // THAT (now reachable via the ND_DEREF string-indexing case
        // below), so a caller asking "is this sizeof provably constant"
        // (e.g. the ND_COND dead-branch check in codegen.c) can still
        // get a definite answer without ever touching the type itself.
        Type *ty = (node->lhs && node->lhs->ty) ? node->lhs->ty : node->ty;
        if (!ty)
            return false;
        if (ty->kind != TY_VLA)
            return (*val = ty->size), true;
        return eval_const_expr(type_size_node(ty, node->tok), val);
    }
    case ND_STR:
        // A string literal used as a value in a constant-expression
        // context (almost always a ternary/logical condition, e.g. the
        // kernel's __SIP_HDR() macro: "(__cname) ? sizeof(__cname) - 1
        // : 0" with __cname substituted by a real "f"/"t"/... literal) —
        // its array-to-pointer-decayed address is never null, so it's
        // always truthy. Not meaningful as an actual numeric *value*
        // (two distinct string literals' addresses aren't a constant
        // relative to each other), but every existing caller of this
        // function only ever needs a literal's truth value here, never
        // its address as a number.
        *val = 1;
        return true;
    case ND_ADDR:
        // &*x = x
        if (node->lhs->kind == ND_DEREF)
            return eval_const_expr(node->lhs->lhs, val);
        // offsetof: &((struct S*)0)->member
        return eval_const_addr_expr(node->lhs, val);
    case ND_DEREF: {
        // String-literal indexing at a constant offset (`"foo"[N]`, or
        // `N["foo"]` -- C allows either operand order since `a[b]`
        // desugars to `*(a+b)`), a GNU-extension-style fold: NOT a
        // strict C integer-constant-expression (6.6p6 excludes string
        // literals from an ICE), so this must never be reachable from a
        // context requiring a genuine ICE (array-declarator sizing keeps
        // classifying this as a real VLA, matching GCC's own frontend --
        // see the ND_SIZEOF lenient-fold above, the only caller that
        // benefits from this case for a VLA dimension chain like ruby's
        // rb_scan_args_count nested-ternary macros walking a format
        // string literal).
        if (node->lhs->kind != ND_ADD)
            return false;
        Node *base = node->lhs->lhs, *idx = node->lhs->rhs;
        if (base->kind != ND_STR && idx->kind == ND_STR) {
            Node *t = base;
            base = idx;
            idx = t;
        }
        // Only a plain (narrow, 1-byte-per-character) string literal:
        // L"..."/u"..."/U"..." store multi-byte code units, not raw
        // bytes, so `base->str[off]` would read the wrong element
        // entirely (found via GCC torture's 20010325-1.c,
        // `L"a" "b"[1] != L'b'`, wrongly folding to a narrow byte read).
        if (base->kind != ND_STR || !base->ty || base->ty->kind != TY_ARRAY ||
            !base->ty->base || base->ty->base->size != 1)
            return false;
        long long off;
        if (!eval_const_expr(idx, &off) || off < 0)
            return false;
        size_t slen = strlen(base->str);
        *val = (off <= (long long)slen) ? (unsigned char)base->str[off] : 0;
        return true;
    }
    case ND_COMMA: {
        // A comma expr is only foldable (no runtime evaluation needed)
        // when BOTH operands are themselves constant; if lhs has side
        // effects (e.g. `c++`), the comma as a whole must stay a live
        // expression node so those side effects still run, even though
        // its *value* is always rhs's value.
        long long discard;
        return eval_const_expr(node->lhs, &discard) && eval_const_expr(node->rhs, val);
    }
    case ND_COND:
        if (!eval_const_expr(node->cond, &lhs))
            return false;
        return eval_const_expr(lhs ? node->then : node->els, val);
    case ND_LVAR:
        // init_val only ever holds a SCALAR constexpr's folded value
        // (set once, e.g. by the "(T){NUM}" scalar-compound-literal
        // path); a struct/union/array-typed is_constexpr var's real
        // data lives in init_data instead (populated by
        // global_initializer()), so init_val is simply never written
        // for one and stays its zero-initialized default. Without this
        // guard, &(aggregate-typed compound literal) folded to the
        // struct's own garbage "value" (0) here -- via eval_const_expr's
        // ND_ADDR -> eval_const_addr_expr -> this case chain -- instead
        // of correctly failing and falling through to extract_reloc()'s
        // genuine address relocation. Found via njs's
        // "(uintptr_t) &(njs_webcrypto_algorithm_t){...}": the anon
        // struct compound literal silently "folded" to address 0.
        // A plain (non-`const`) global with a literal initializer is NOT
        // a compile-time constant just because it starts one -- it's an
        // ordinary mutable object (see the matching ND_MEMBER case's
        // comment for the concrete bash bug this guards against).
        // Require real const-qualification (ty_const) for a non-local
        // var, or an explicit constexpr, matching real GCC/Clang's
        // extension of accepting `const int j = i;` for an already-
        // initialized `const int i` (GCC PR99577: ISO C doesn't require
        // this, but every mainstream compiler does it).
        if (node->var && ((node->var->is_constexpr) ||
            (!node->var->is_local && ty_const(node->var->ty))) && node->var->has_init &&
            (!node->ty || (node->ty->kind != TY_STRUCT && node->ty->kind != TY_UNION && node->ty->kind != TY_ARRAY))) {
            *val = node->var->init_val;
            return true;
        }
        return false;
    case ND_MEMBER:
        // constexpr struct member access: evaluate base, add member offset
        {
            // base_val now holds the value at the base offset;
            // Try the direct path: find the root LVar and accumulate offsets.
            Node *cur = node;
            int total_off = 0;
            LVar *root_var = NULL;
            // memcpy-based extraction below reads whole bytes at
            // `member->offset`; a bitfield's real value lives at
            // `bit_offset` *within* that storage unit and needs a
            // shift+mask this fold doesn't do, so any bitfield in the
            // chain must disable the fold entirely rather than silently
            // return the wrong (unshifted, unmasked) value. Real bug:
            // struct-ini-2.c/bitfld-3.c's plain (non-const) global
            // bitfield structs were "folded" to garbage the moment an
            // `if` condition reading one became foldable (see the ND_IF
            // caller in opt.c).
            bool is_bitfield = false;
            while (cur && cur->kind == ND_MEMBER) {
                if (cur->member) {
                    total_off += cur->member->offset;
                    if (cur->member->bit_width > 0) is_bitfield = true;
                }
                cur = cur->lhs;
            }
            if (cur && cur->kind == ND_LVAR) {
                root_var = cur->var;
            }
            // A plain (non-`const`) global with a literal initializer, like
            // bash's `struct dstack dstack = { NULL, 0, 0 };`, is NOT a
            // compile-time constant just because it *starts* one — it's an
            // ordinary mutable object the rest of the program writes to at
            // runtime (`dstack.delimiter_depth++` and friends). Folding a
            // `dstack.delimiter_depth`-style member read to the *static
            // initializer's* value here permanently blinds every such read,
            // anywhere in the translation unit, to every later write —
            // `!root_var->is_local` alone must never stand in for "provably
            // never written again"; require real const-qualification (or an
            // explicit `constexpr`/compound-literal `is_constexpr`) instead.
            // Real bug: this let `current_delimiter(dstack) == '\''`-style
            // checks throughout bash's hand-written parser (parse.y) fold to
            // a permanent `false`, breaking quote-state tracking badly
            // enough that alias-expansion recursion no longer terminated —
            // hung test/third_party/test_httpparser's sibling
            // test/third_party/test_bash at any `-O1`+ build.
            if (!is_bitfield && root_var && (root_var->is_constexpr || (!root_var->is_local && ty_const(root_var->ty))) && root_var->has_init) {
                if (root_var->init_data && is_integer(node->ty)) {
                    int64_t v = 0;
                    memcpy(&v, root_var->init_data + total_off, node->ty->size <= 8 ? node->ty->size : 8);
                    if (!node->ty->is_unsigned && (v >> (node->ty->size * 8 - 1)))
                        v |= ~((1ULL << (node->ty->size * 8)) - 1);
                    *val = v;
                    return true;
                }
                if (root_var->has_init && !root_var->init_data) {
                    // Scalar constexpr: just return init_val (members share the scalar)
                    *val = root_var->init_val;
                    return true;
                }
            }
            // Fallback: compound literal anon vars have is_local=true but is_constexpr=true
            // and init_data populated by global_initializer. Read member value from init_data.
            if (!is_bitfield && is_integer(node->ty) && total_off >= 0) {
                if (!root_var && cur && cur->kind == ND_COMMA) {
                    // Find root LVar in the comma chain
                    Node *st[64];
                    int sp = 0;
                    st[sp++] = cur;
                    while (sp > 0 && !root_var) {
                        Node *n = st[--sp];
                        if (n->kind == ND_LVAR && n->var && n->var->is_local)
                            root_var = n->var;
                        if (n->kind == ND_COMMA) {
                            if (n->lhs && sp < 64) st[sp++] = n->lhs;
                            if (n->rhs && sp < 64) st[sp++] = n->rhs;
                        }
                    }
                }
                if (root_var && root_var->is_constexpr && root_var->init_data) {
                    int64_t v = 0;
                    int read_sz = node->ty->size <= 8 ? node->ty->size : 8;
                    if (total_off + read_sz <= root_var->init_size) {
                        memcpy(&v, root_var->init_data + total_off, read_sz);
                        if (!node->ty->is_unsigned && (v >> (read_sz * 8 - 1)))
                            v |= ~((1ULL << (read_sz * 8)) - 1);
                        *val = v;
                        return true;
                    }
                }
            }
            return false;
        }
    case ND_FUNCALL: {
        // __builtin_{add,sub,mul}_overflow_p(a, b, (__typeof__(a op b))0)
        // is GCC/Clang's genuinely constant-foldable overflow-predicate
        // builtin (unlike the 3-pointer-argument __builtin_*_overflow,
        // which has a store side effect and is never a valid constant
        // expression). gnulib's intprops.h picks this exact form whenever
        // __has_builtin reports it available -- and rcc's __has_builtin
        // table (preprocess.c) already claims support, gated on
        // `__GNUC__ || __clang__`, the same condition gnulib's own
        // test-intprops.c uses to select `static_assert` over a runtime
        // ASSERT. Runtime codegen (cg_builtins.c) already implements
        // these correctly; without this fold, any `static_assert` or
        // array-size use of them (real-world overflow-checked constant
        // computations, plus gnulib's own self-tests) failed with
        // "condition must be a constant expression" even though every
        // operand was already fully constant.
        if (node->funcname &&
            (node->funcname == bi_add_overflow_p || node->funcname == bi_sub_overflow_p ||
             node->funcname == bi_mul_overflow_p)) {
            Node *a = node->args;
            Node *b = a ? a->next : NULL;
            Node *c = b ? b->next : NULL;
            if (!a || !b || !c || c->next)
                return false;
            long long av, bv;
            if (!eval_const_expr(a, &av) || !eval_const_expr(b, &bv))
                return false;
            Type *rty = c->ty;
            if (!rty || !is_integer(rty) || rty->size <= 0 || rty->size > 8)
                return false;
            int bits = rty->size * 8;
            __int128 amin, amax;
            if (rty->is_unsigned) {
                amin = 0;
                amax = bits >= 64 ? (__int128)((unsigned long long)-1) : (((__int128)1 << bits) - 1);
            } else {
                amax = (bits >= 64 ? (__int128)0x7FFFFFFFFFFFFFFFLL : (((__int128)1 << (bits - 1)) - 1));
                amin = -amax - 1;
            }
            // GCC's contract (__builtin_*_overflow_p docs): the two value
            // operands are promoted to infinite-precision SIGNED math
            // using THEIR OWN type's value -- not reinterpreted through
            // the result type first. A negative `int` operand stays
            // negative even when the result type is unsigned (e.g.
            // INT_MIN * ULONG_MAX must be evaluated as the huge negative
            // product -2147483648 * 18446744073709551615, not as
            // (unsigned long)INT_MIN * ULONG_MAX). eval_const_expr()
            // already returns each operand's raw two's-complement 64-bit
            // pattern per ITS OWN type (e.g. ULONG_MAX comes back as the
            // bit pattern -1); reinterpret it as unsigned here whenever
            // that operand's own type is unsigned, so the __int128 widen
            // recovers the true mathematical value in both cases.
            __int128 A = (a->ty && a->ty->is_unsigned) ? (__int128)(unsigned long long)av : (__int128)av;
            __int128 B = (b->ty && b->ty->is_unsigned) ? (__int128)(unsigned long long)bv : (__int128)bv;
            __int128 r;
            if (node->funcname == bi_add_overflow_p) r = A + B;
            else if (node->funcname == bi_sub_overflow_p)
                r = A - B;
            else
                r = A * B;
            *val = (r < amin || r > amax) ? 1 : 0;
            return true;
        }
        // Fold constant-argument calls to bit-counting builtins. Kernel
        // (and other) code computes compile-time bit widths via
        // __builtin_constant_p(n) ? ... clz/ctz/popcount(n) ... : runtime_fn(n)
        // (see linux/log2.h bits_per()/ilog2()); once __builtin_constant_p
        // has already folded the condition to a literal 1, this branch must
        // itself fold to a constant for the enclosing static_assert to hold.
        if (!node->args || node->args->next)
            return false;
        // node->funcname is only set for a call resolved as an
        // as-yet-undeclared "plain identifier" (the usual path for these
        // builtins). If the callee happens to already have a real
        // prototype in scope (e.g. <linux/string.h> declaring
        // "extern size_t strlen(const char *)" — __builtin_strlen itself
        // is preprocessor-macro'd to plain "strlen", see preprocess.c's
        // define_pre("__builtin_strlen", "strlen")), the call instead
        // resolves through node->lhs referencing that declared LVar, and
        // funcname is never populated. Fall back to the declared symbol's
        // own name so both paths reach the same fold below.
        char *fn = node->funcname;
        if (!fn && node->lhs && node->lhs->kind == ND_LVAR && node->lhs->var)
            fn = node->lhs->var->name;
        if (!fn)
            return false;
        // strlen(STR_LITERAL) is GCC/Clang's other well-known
        // constant-foldable-call extension, relied on throughout the
        // kernel for "_Static_assert(sizeof(lit) - 1 == strlen(lit), \"no
        // embedded NUL\")" (MODULE_INFO() et al. via linux/stringify.h).
        // Its argument is a string, not an integer, so it must be checked
        // before the integer-only eval_const_expr() call below rejects it.
        if (fn == bi_strlen || fn == bi_s_strlen) {
            Node *a = node->args;
            while (a && a->kind == ND_CAST) a = a->lhs;
            if (a && a->kind == ND_STR) {
                *val = (long long)strlen(a->str);
                return true;
            }
            return false;
        }
        long long arg;
        if (!eval_const_expr(node->args, &arg))
            return false;
        // node->funcname is tok->name for a plain identifier call, which the
        // lexer already str_intern's — compare against the pre-interned
        // bi_* pointers (init_builtin_names(), cg_builtins.c) instead of
        // strcmp, same convention used everywhere else these are checked.
        unsigned uv32 = (unsigned)arg;
        unsigned long long uv64 = (unsigned long long)arg;
        if (fn == bi_clz) {
            if (uv32 == 0) return false;
            int n = 0;
            while (!(uv32 & 0x80000000u)) {
                uv32 <<= 1;
                n++;
            }
            *val = n;
            return true;
        }
        if (fn == bi_clzl && ty_long->size == 4) {
            if (uv32 == 0) return false;
            int n = 0;
            while (!(uv32 & 0x80000000u)) {
                uv32 <<= 1;
                n++;
            }
            *val = n;
            return true;
        }
        if (fn == bi_clzl || fn == bi_clzll) {
            if (uv64 == 0) return false;
            int n = 0;
            while (!(uv64 & (1ULL << 63))) {
                uv64 <<= 1;
                n++;
            }
            *val = n;
            return true;
        }
        if (fn == bi_ctz) {
            if (uv32 == 0) return false;
            int n = 0;
            while (!(uv32 & 1)) {
                uv32 >>= 1;
                n++;
            }
            *val = n;
            return true;
        }
        if (fn == bi_ctzl && ty_long->size == 4) {
            if (uv32 == 0) return false;
            int n = 0;
            while (!(uv32 & 1)) {
                uv32 >>= 1;
                n++;
            }
            *val = n;
            return true;
        }
        if (fn == bi_ctzl || fn == bi_ctzll) {
            if (uv64 == 0) return false;
            int n = 0;
            while (!(uv64 & 1)) {
                uv64 >>= 1;
                n++;
            }
            *val = n;
            return true;
        }
        if (fn == bi_popcount) {
            int n = 0;
            while (uv32) {
                n += (int)(uv32 & 1);
                uv32 >>= 1;
            }
            *val = n;
            return true;
        }
        if (fn == bi_popcountl || fn == bi_popcountll) {
            int n = 0;
            while (uv64) {
                n += (int)(uv64 & 1);
                uv64 >>= 1;
            }
            *val = n;
            return true;
        }
        if (fn == bi_ffs) {
            if (uv32 == 0) {
                *val = 0;
                return true;
            }
            int n = 1;
            while (!(uv32 & 1)) {
                uv32 >>= 1;
                n++;
            }
            *val = n;
            return true;
        }
        if (fn == bi_ffsl || fn == bi_ffsll) {
            if (uv64 == 0) {
                *val = 0;
                return true;
            }
            int n = 1;
            while (!(uv64 & 1)) {
                uv64 >>= 1;
                n++;
            }
            *val = n;
            return true;
        }
        // __builtin_bswap16/32/64 must constant-fold too: the kernel's
        // __swab16/32/64 (linux/swab.h) expand to these when
        // __HAVE_BUILTIN_BSWAP*__ is set, and are used as case labels /
        // in static_assert throughout networking headers (netdevice.h).
        if (fn == bi_bswap16) {
            *val = (uint16_t)(((uv32 & 0xff) << 8) | ((uv32 >> 8) & 0xff));
            return true;
        }
        if (fn == bi_bswap32) {
            uint32_t v = (uint32_t)uv32;
            *val = (uint32_t)(((v & 0x000000ffu) << 24) | ((v & 0x0000ff00u) << 8) |
                              ((v & 0x00ff0000u) >> 8) | ((v & 0xff000000u) >> 24));
            return true;
        }
        if (fn == bi_bswap64) {
            uint64_t v = uv64;
            *val = (int64_t)(uint64_t)(((v & 0x00000000000000ffULL) << 56) | ((v & 0x000000000000ff00ULL) << 40) |
                                       ((v & 0x0000000000ff0000ULL) << 24) | ((v & 0x00000000ff000000ULL) << 8) |
                                       ((v & 0x000000ff00000000ULL) >> 8) | ((v & 0x0000ff0000000000ULL) >> 24) |
                                       ((v & 0x00ff000000000000ULL) >> 40) | ((v & 0xff00000000000000ULL) >> 56));
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

// Public entry point: fold, then truncate/sign-extend the result to the
// node's own type width. eval_const_expr_impl's individual ND_ADD/SUB/MUL/
// BITAND/... cases compute raw 64-bit `long long` arithmetic with no
// truncation of their own (only ND_CAST explicitly masks) -- correct for
// any type that's already 8 bytes wide, but WRONG the moment the node's
// real type is narrower, e.g. `unsigned int`: `0u - 1` must wrap to
// UINT_MAX (0xFFFFFFFF), not stay the 64-bit pattern -1
// (0xFFFFFFFFFFFFFFFF) -- a later `>>` on that value inspects
// node->lhs->ty->is_unsigned and shifts the WRONG 64-bit pattern,
// producing a value with the top 33 bits still set instead of 0.
// Applying this once per node (recursive calls all route through this
// wrapper, not eval_const_expr_impl directly) truncates every
// intermediate value exactly where real arithmetic would, matching both
// runtime codegen and the C abstract machine. Found via gnulib's
// intprops.h: `_GL_INT_NEGATE_CONVERT(UINT_MAX, 1)` (an unsigned-int-typed
// "(1 ? 0 : e) - 1" ghost-ternary idiom) folded to a 64-bit -1 instead of
// UINT_MAX, corrupting every INT_LEFT_SHIFT_OVERFLOW-style compile-time
// overflow check built on top of it.
bool eval_const_expr(Node *node, long long *val) {
    if (!eval_const_expr_impl(node, val))
        return false;
    if (!node->ty)
        return true;
    if (node->ty->kind == TY_BOOL) {
        *val = *val != 0;
        return true;
    }
    if (is_integer(node->ty) && node->ty->kind != TY_BITINT) {
        int sz = node->ty->size;
        if (sz > 0 && sz < 8) {
            int bits = sz * 8;
            unsigned long long mask = (1ULL << bits) - 1;
            unsigned long long uv = (unsigned long long)*val & mask;
            if (!node->ty->is_unsigned && (uv & (1ULL << (bits - 1))))
                uv |= ~mask;
            *val = (long long)uv;
        }
    }
    return true;
}

static bool eval_double_const_expr(Node *node, double *val) {
    double lhs, rhs;
    if (!node)
        return false;
    // codeql[cpp/long-switch]: central AST-node-kind dispatch; splitting cases into helpers is a large, purely-cosmetic refactor of core compiler internals, not attempted here.
    switch (node->kind) {
    case ND_FNUM:
        *val = node->fval;
        return true;
    case ND_NUM:
        *val = (double)node->val;
        return true;
    case ND_ADD:
        if (fenv_access && node->ty && is_flonum(node->ty)) return false;
        if (!eval_double_const_expr(node->lhs, &lhs) || !eval_double_const_expr(node->rhs, &rhs)) return false;
        *val = lhs + rhs;
        if (node->ty && node->ty->kind == TY_FLOAT) *val = (float)*val;
        return true;
    case ND_SUB:
        if (fenv_access && node->ty && is_flonum(node->ty)) return false;
        if (!eval_double_const_expr(node->lhs, &lhs) || !eval_double_const_expr(node->rhs, &rhs)) return false;
        *val = lhs - rhs;
        if (node->ty && node->ty->kind == TY_FLOAT) *val = (float)*val;
        return true;
    case ND_MUL:
        if (fenv_access && node->ty && is_flonum(node->ty)) return false;
        if (!eval_double_const_expr(node->lhs, &lhs) || !eval_double_const_expr(node->rhs, &rhs)) return false;
        *val = lhs * rhs;
        if (node->ty && node->ty->kind == TY_FLOAT) *val = (float)*val;
        return true;
    case ND_DIV:
        if (fenv_access && node->ty && is_flonum(node->ty)) return false;
        if (!eval_double_const_expr(node->lhs, &lhs) || !eval_double_const_expr(node->rhs, &rhs)) return false;
        if (node->ty && is_integer(node->ty)) {
            // Integer division truncates toward zero (C11 6.5.5p6), unlike
            // this function's usual double-precision arithmetic -- an
            // int-typed sub-expression buried inside a larger float
            // constant expression (e.g. a fixed-point "(A * B) / C"
            // computed in integer math, then divided by a float literal
            // to get a real-world unit) must discard its fractional part
            // right here, or the caller folding the outer float division
            // silently gets a materially different value: this exact
            // pattern -- `((100 * 60 * 1000) / 1001) / 100.0f` computing
            // an NTSC 59.94 fps constant -- previously folded the inner
            // integer division as plain double math (6000000.0/1001.0 =
            // 5994.0059..., never truncated to the correct 5994), then
            // divided by 100.0f to land on a float noticeably off from
            // the runtime-computed value of the identical expression.
            if (rhs == 0.0) return false; // division by zero: not a valid constant expression
            long long il = (long long)lhs, ir = (long long)rhs;
            *val = (double)(il / ir);
        } else {
            *val = lhs / rhs;
        }
        if (node->ty && node->ty->kind == TY_FLOAT) *val = (float)*val;
        return true;
    case ND_NEG:
        if (fenv_access && node->ty && is_flonum(node->ty)) return false;
        return eval_double_const_expr(node->lhs, &lhs) && ((*val = -lhs), true);
    case ND_CAST:
        if (fenv_access && node->ty && is_flonum(node->ty)) return false;
        if (fenv_access && node->lhs && node->lhs->ty && is_flonum(node->lhs->ty)) return false;
        if (!eval_double_const_expr(node->lhs, val)) return false;
        if (node->ty && node->ty->kind == TY_FLOAT) *val = (float)*val;
        if (node->ty && is_integer(node->ty)) *val = (double)(int64_t)*val;
        return true;
    // codeql[cpp/equality-on-floats]: this implements C's own ==/!=
    // operators for float constant-folding — must be bit-exact IEEE754
    // comparison to match what the same expression evaluates to at
    // runtime, not a fuzzy/epsilon comparison.
    case ND_EQ:
        return eval_double_const_expr(node->lhs, &lhs) && eval_double_const_expr(node->rhs, &rhs) && ((*val = lhs == rhs), true);
    // codeql[cpp/equality-on-floats]: same as ND_EQ above.
    case ND_NE:
        return eval_double_const_expr(node->lhs, &lhs) && eval_double_const_expr(node->rhs, &rhs) && ((*val = lhs != rhs), true);
    case ND_LT:
        return eval_double_const_expr(node->lhs, &lhs) && eval_double_const_expr(node->rhs, &rhs) && ((*val = lhs < rhs), true);
    case ND_LE:
        return eval_double_const_expr(node->lhs, &lhs) && eval_double_const_expr(node->rhs, &rhs) && ((*val = lhs <= rhs), true);
    case ND_LVAR:
        if (node->var && node->var->is_constexpr && node->var->has_init) {
            if (node->var->init_data && node->ty && is_flonum(node->ty)) {
                double fv = 0;
                memcpy(&fv, node->var->init_data, node->ty->size <= 8 ? node->ty->size : 8);
                *val = fv;
                return true;
            }
        }
        return false;
    case ND_MEMBER: {
        int total_off = 0;
        Node *cur = node;
        while (cur && cur->kind == ND_MEMBER) {
            if (cur->member) total_off += cur->member->offset;
            cur = cur->lhs;
        }
        LVar *root_var = NULL;
        if (cur && cur->kind == ND_LVAR && cur->var)
            root_var = cur->var;
        if (!root_var && cur && cur->kind == ND_COMMA) {
            Node *st[64];
            int sp = 0;
            st[sp++] = cur;
            while (sp > 0 && !root_var) {
                Node *n = st[--sp];
                if (n->kind == ND_LVAR && n->var && n->var->is_local)
                    root_var = n->var;
                if (n->kind == ND_COMMA) {
                    if (n->lhs && sp < 64) st[sp++] = n->lhs;
                    if (n->rhs && sp < 64) st[sp++] = n->rhs;
                }
            }
        }
        if (root_var && root_var->is_constexpr && root_var->init_data) {
            double fv = 0;
            int read_sz = node->ty->size <= 8 ? node->ty->size : 8;
            if (total_off + read_sz <= root_var->init_size) {
                memcpy(&fv, root_var->init_data + total_off, read_sz);
                *val = fv;
                return true;
            }
        }
        return false;
    }
    default: {
        // A double-typed initializer whose value is a purely-integer
        // constant expression this switch doesn't otherwise fold (shifts,
        // bitwise ops, mod, ...) -- e.g. njs's "NJS_MAX_SAFE_INTEGER"
        // (`(njs_int64_t) ((1LL << 53) - 1)`) assigned to a `double`
        // struct member. C's usual arithmetic/assignment conversions
        // implicitly convert any integer constant to the target floating
        // type, so fall back to the integer evaluator whenever the node's
        // own type isn't itself a float (a genuine flonum operand that
        // reached here has already failed every float-op case above and
        // must not silently truncate through the integer path).
        if (node->ty && !is_flonum(node->ty)) {
            long long ival;
            if (eval_const_expr(node, &ival)) {
                *val = node->ty->is_unsigned ? (double)(unsigned long long)ival : (double)ival;
                return true;
            }
        }
        return false;
    }
    }
}

// Evaluate a complex constant expression, extracting real and imag parts.
// Handles patterns like "1.0 + 1.0i", "-2.0 + 2.0i", "1.0 + 14.0 * (1.0fi)", etc.
static bool eval_complex_const_expr(Node *node, double *real_out, double *imag_out) {
    double rl, il, rr, ir;
    if (!node) return false;
    // codeql[cpp/long-switch]: central AST-node-kind dispatch; splitting cases into helpers is a large, purely-cosmetic refactor of core compiler internals, not attempted here.
    switch (node->kind) {
    case ND_FNUM:
        *real_out = node->fval;
        *imag_out = 0.0;
        return true;
    case ND_NUM:
        *real_out = (double)node->val;
        *imag_out = 0.0;
        return true;
    case ND_ADD:
        if (!eval_complex_const_expr(node->lhs, &rl, &il)) return false;
        if (!eval_complex_const_expr(node->rhs, &rr, &ir)) return false;
        *real_out = rl + rr;
        *imag_out = il + ir;
        return true;
    case ND_SUB:
        if (!eval_complex_const_expr(node->lhs, &rl, &il)) return false;
        if (!eval_complex_const_expr(node->rhs, &rr, &ir)) return false;
        *real_out = rl - rr;
        *imag_out = il - ir;
        return true;
    case ND_MUL:
        if (!eval_complex_const_expr(node->lhs, &rl, &il)) return false;
        if (!eval_complex_const_expr(node->rhs, &rr, &ir)) return false;
        *real_out = rl * rr - il * ir;
        *imag_out = rl * ir + il * rr;
        return true;
    case ND_DIV:
        if (!eval_complex_const_expr(node->lhs, &rl, &il)) return false;
        if (!eval_complex_const_expr(node->rhs, &rr, &ir)) return false;
        {
            double denom = rr * rr + ir * ir;
            if (denom == 0.0) return false;
            *real_out = (rl * rr + il * ir) / denom;
            *imag_out = (il * rr - rl * ir) / denom;
        }
        return true;
    case ND_NEG:
        if (!eval_complex_const_expr(node->lhs, &rl, &il)) return false;
        *real_out = -rl;
        *imag_out = -il;
        return true;
    case ND_CAST:
        return eval_complex_const_expr(node->lhs, real_out, imag_out);
    case ND_COMMA: {
        // Walk through comma chains (e.g., from new_complex_val:
        // ND_COMMA(ND_COMMA(ND_ASSIGN(ND_REAL, real), ND_ASSIGN(ND_IMAG, imag)), ND_LVAR))
        // Extract real/imag from the ND_ASSIGN nodes in the chain.
        double rv = 0.0, iv = 0.0;
        bool has_real = false, has_imag = false;
        Node *inner = node;
        while (inner->kind == ND_COMMA) {
            Node *l = inner->lhs;
            if (l->kind == ND_COMMA) {
                double cr, ci;
                if (eval_complex_const_expr(l, &cr, &ci)) {
                    rv = cr;
                    has_real = true;
                    iv = ci;
                    has_imag = true;
                }
            } else if (l->kind == ND_ASSIGN && l->lhs) {
                if (l->lhs->kind == ND_REAL) {
                    if (eval_double_const_expr(l->rhs, &rv)) has_real = true;
                } else if (l->lhs->kind == ND_IMAG) {
                    if (eval_double_const_expr(l->rhs, &iv)) has_imag = true;
                }
            }
            inner = inner->rhs;
        }
        if (inner->kind == ND_ASSIGN && inner->lhs) {
            if (inner->lhs->kind == ND_REAL && eval_double_const_expr(inner->rhs, &rv)) has_real = true;
            else if (inner->lhs->kind == ND_IMAG && eval_double_const_expr(inner->rhs, &iv))
                has_imag = true;
        }
        if (has_real || has_imag) {
            *real_out = rv;
            *imag_out = iv;
            return true;
        }
        return eval_complex_const_expr(node->rhs, real_out, imag_out);
    }
    default:
        return false;
    }
}


static Type *declarator(Token **rest, Token *tok, Type *ty, char **name, VarAttr *attr);

// Parse zero or more `pre(EXPR)` / `post([IDENT ':'] EXPR)` contract
// specifiers trailing a function declarator's parameter list — Gustedt's
// "Contracts for C" (https://gustedt.wordpress.com/2025/03/10/contracts-for-c/),
// minus the pre()/post() *statement* forms (issue #45: no semicolon-
// terminated pre()/post() statements, so the whole thing can be hidden
// behind a feature-test macro for compilers without contract support —
// `#define pre(...)` / `#define post(...)` on those compilers is enough,
// since a specifier never needs its own terminating token).
//
// Deliberately captures only the raw token span here — no expr()/
// conditional() call: the parameter placeholder LVars that would resolve
// identifiers are about to go out of scope (see declarator_params'
// `locals = saved_locals` below), and post()'s return-value binding has
// no local to bind to yet. Conditions are compiled for real, once per
// point of use, against that use's own parameter/binding locals — see
// activate_function_contracts() and apply_postconds_to_return().
//
// `pre`/`post` are ordinary identifiers, not keywords: recognized only in
// this exact trailing position, mirroring the pre-existing generic
// `IDENT (...)` specifier skip for block-scope prototypes (e.g. GCC's
// `__cond_acquires(...)`, see declaration()).
static Token *parse_contract_specs(Token *tok, Contract **pre_out, Contract **post_out) {
    Contract pre_head = {0}, post_head = {0};
    Contract *pre_cur = &pre_head, *post_cur = &post_head;
    for (;;) {
        if (tok->kind != TK_IDENT || !equalc(tok->next, "(") ||
            !(equalc(tok, "pre") || equalc(tok, "post")))
            break;
        bool is_post = equalc(tok, "post");
        Token *kw_tok = tok;
        Token *paren = tok->next;
        int depth = 0;
        Token *scan = paren;
        for (;;) {
            if (equalc(scan, "("))
                depth++;
            else if (equalc(scan, ")")) {
                depth--;
                if (depth == 0)
                    break;
            }
            if (scan->kind == TK_EOF)
                error_tok(scan, "unterminated '%s(...)' contract specifier", is_post ? "post" : "pre");
            scan = scan->next;
        }
        // scan is now at the matching ')'
        Token *cond_start = paren->next;
        char *bind_name = NULL;
        if (is_post && cond_start != scan && cond_start->kind == TK_IDENT && equalc(cond_start->next, ":")) {
            bind_name = cond_start->name;
            cond_start = cond_start->next->next;
        }
        if (cond_start == scan)
            error_tok(cond_start, "expected a condition in '%s(...)'", is_post ? "post" : "pre");
        Contract *c = arena_alloc(sizeof(Contract));
        c->tok = kw_tok;
        c->cond_start = cond_start;
        c->cond_end = scan;
        c->bind_name = bind_name;
        c->next = NULL;
        if (is_post)
            post_cur = post_cur->next = c;
        else
            pre_cur = pre_cur->next = c;
        tok = scan->next;
    }
    *pre_out = pre_head.next;
    *post_out = post_head.next;
    return tok;
}

static Type *declarator_params(Token **rest, Token *tok, Type *ty) {
    Type param_head = {};
    Type *pcur = &param_head;
    bool is_variadic = false;
    // Save locals so earlier params are visible during VLA dim expressions
    // (e.g. void foo(int a, int b[a++])), then restore afterward.
    LVar *saved_locals = locals;

    if (equalc(tok, "void") && equalc(tok->next, ")")) {
        tok = tok->next->next;
        locals = saved_locals;
        ty = func_type(ty);
        ty->param_types = NULL;
        ty->is_variadic = false;
        ty->is_void_params = true;
        tok = parse_contract_specs(tok, &ty->preconds, &ty->postconds);
        *rest = tok;
        return ty;
    } else {
        while (!equalc(tok, ")")) {
            if (pcur != &param_head)
                tok = skip(tok, ",");
            if (equalc(tok, "...")) {
                is_variadic = true;
                tok = tok->next;
                break;
            }
            if (equalc(tok, ";")) {
                tok = tok->next;
                continue;
            }

            VarAttr attr = {};
            Type *base = declspec(&tok, tok, &attr);
            char *pname = NULL;
            Type *pty = declarator(&tok, tok, copy_type(base), &pname, NULL);
            // C11 6.7.4p2: _Noreturn only on function declarations
            if (attr.is_noreturn_std)
                error_tok(tok, "'_Noreturn' on function parameter");
            if (attr.has_alignas)
                error_tok(tok, "alignment specified for parameter");
            tok = skip_attributes(tok);

            // Preserve VLA dim expression from single-dimension VLA param (e.g. b[a++])
            // so side effects can be emitted at function entry.
            Node *vla_dim_expr = NULL;
            // C11 6.7.6.3p7: a qualifier inside the [] of the outermost
            // array derivation qualifies the decayed pointer PARAMETER
            // itself (`int a[const]` -> `int *const a`), stashed by
            // type_suffix() on the array/VLA type's own ->qual (arrays
            // otherwise never use it) — apply it to the pointer, not the
            // pointee, which already carries its own (possibly qualified)
            // element type via pty->base.
            if (pty->kind == TY_VLA) {
                vla_dim_expr = pty->vla_len_expr;
                unsigned char pqual = pty->qual;
                pty = pointer_to(pty->base);
                pty->qual |= pqual;
            } else if (pty->kind == TY_ARRAY) {
                unsigned char pqual = pty->qual;
                pty = pointer_to(pty->base);
                pty->qual |= pqual;
            } else if (pty->kind == TY_FUNC) {
                pty = pointer_to(pty);
            }

            Type *pt = arena_alloc(sizeof(Type));
            *pt = *pty;
            pt->param_next = NULL;
            pt->name = pname;
            if (vla_dim_expr)
                pt->vla_len_expr = vla_dim_expr;

            // Create placeholder LVar for this param so subsequent params can
            // reference it in VLA dimension expressions. Store it in vla_len_val
            // so the function definition handler can reuse it with the correct offset.
            if (pname) {
                LVar *plvar = arena_alloc(sizeof(LVar));
                plvar->name = pname;
                // Use pty (the original, identity-preserving type), not pt
                // (the shallow-copied param_types list node): pt's `*pt =
                // *pty` clone breaks type_equal()'s pointer-identity check
                // for struct/union types, e.g. i915_reg_t/i915_mcr_reg_t
                // (drivers/gpu/drm/i915/i915_reg_defs.h) — two distinct
                // anonymous single-u32-member structs distinguished only
                // by tag identity, selected via
                // `_Generic((r), i915_reg_t: ..., i915_mcr_reg_t: ...)`.
                // With plvar->ty pointing at the clone instead of the
                // typedef table's own i915_reg_t Type*, neither
                // association's re-resolved "i915_reg_t"/"i915_mcr_reg_t"
                // type_name() result was ever pointer-equal to it, so
                // _Generic always fell through to "no matching
                // association" for every struct-typed parameter reference.
                // pt and pty have identical size/align/members regardless,
                // so this doesn't affect offset/alignment computation.
                plvar->ty = pty;
                plvar->is_local = true;
                plvar->offset = 0; // placeholder; updated by definition handler
                plvar->next = locals;
                locals = plvar;
                pt->vla_len_val = plvar;
            }

            pcur = pcur->param_next = pt;
        }
        tok = skip(tok, ")");
    }

    // Restore locals (placeholder LVars remain in arena, referenced by pt->vla_len_val)
    locals = saved_locals;

    ty = func_type(ty);
    ty->param_types = param_head.param_next;
    ty->is_variadic = is_variadic;
    tok = parse_contract_specs(tok, &ty->preconds, &ty->postconds);
    *rest = tok;
    return ty;
}

static Type *type_suffix(Token **rest, Token *tok, Type *ty, char *decl_name) {
    int64_t dims[16];
    Node *vla_exprs[16] = {0};
    int ndims = 0;
    // C11 6.7.6.3p4/p7: type qualifiers (and 'static') inside the []
    // of a parameter's array declarator apply only in the OUTERMOST
    // array derivation (the first bracket in source order — see the
    // "apply dimensions" comment below for why dims[0] is outermost),
    // and mean the DECAYED POINTER PARAMETER ITSELF is so-qualified
    // (`int a[const]` -> `int *const a`), not its pointee. Recorded
    // here and consumed by declarator_params()'s array/VLA decay.
    unsigned char dim0_qual = 0;
    while (equalc(tok, "[") && !(equalc(tok->next, "[") && tok->ptr + tok->len == tok->next->ptr)) {
        tok = tok->next;
        int64_t len = 0;
        Node *vla_expr = NULL;
        unsigned char dim_qual = 0;
        for (;;) {
            if (equalc(tok, "const") || equalc(tok, "__const") || equalc(tok, "__const__"))
                dim_qual |= QUAL_CONST;
            else if (equalc(tok, "volatile") || equalc(tok, "__volatile") || equalc(tok, "__volatile__"))
                dim_qual |= QUAL_VOLATILE;
            else if (equalc(tok, "restrict") || equalc(tok, "__restrict") || equalc(tok, "__restrict__"))
                dim_qual |= QUAL_RESTRICT;
            else if (equalc(tok, "_Atomic") || equalc(tok, "__Atomic"))
                dim_qual |= QUAL_ATOMIC;
            else if (equalc(tok, "static"))
                /* nothing to record for typing purposes */;
            else
                break;
            tok = tok->next;
        }
        if (ndims == 0)
            dim0_qual = dim_qual;
        if (!equalc(tok, "]")) {
            if (equalc(tok, "[")) {
                // nested array declarator [ [ ] ] — skip to matching outer ]
                int bdepth = 1;
                tok = tok->next;
                while (bdepth > 0 && tok) {
                    if (equalc(tok, "[")) bdepth++;
                    else if (equalc(tok, "]"))
                        bdepth--;
                    if (bdepth > 0) tok = tok->next;
                }
                len = 0;
                // tok now at inner ]; skip to outer ] and consume it
                if (tok) tok = tok->next;
                if (equalc(tok, "]")) tok = tok->next;
                dims[ndims] = len;
                vla_exprs[ndims] = vla_expr;
                ndims++;
                continue;
            } else if (equalc(tok, "*") && equalc(tok->next, "]")) {
                // C99 [*] unspecified-size VLA marker (valid only in a
                // function prototype). Only when the '*' is immediately
                // followed by ']'; otherwise it starts a size expression
                // such as [*n] (a VLA sized by a pointer dereference, e.g.
                // `int p[*count]`), which must fall through to expr() below.
                tok = tok->next;
            } else {
                Node *node = expr(&tok, tok);
                long long val = 0;
                if (eval_const_expr(node, &val)) {
                    if (val < 0) {
                        if (decl_name)
                            error_tok(node->tok, "size of array '%s' is negative", decl_name);
                        else
                            error_tok(node->tok, "size of array is negative");
                    }
                    len = val;
                } else {
                    len = -1;
                    vla_expr = node;
                }
            }
        }
        tok = skip(tok, "]");
        // Consume C23 [[]] attributes between array dimensions
        tok = read_type_attrs(tok, NULL, NULL);
        if (ndims >= 16)
            error_tok(tok, "too many array dimensions");
        dims[ndims] = len;
        vla_exprs[ndims] = vla_expr;
        ndims++;
    }
    // Apply dimensions from innermost (rightmost in source) to outermost
    for (int i = ndims - 1; i >= 0; i--) {
        if (vla_exprs[i])
            ty = vla_of(ty, vla_exprs[i], 0);
        else
            ty = array_of(ty, dims[i]);
    }
    // The outermost array/VLA type (built last above, from dims[0]) is
    // exactly what declarator_params() decays to a pointer — stash the
    // parameter-array qualifier on it (arrays otherwise never use ->qual)
    // so the decay step can apply it to the pointer itself.
    if (dim0_qual && ndims > 0)
        ty->qual |= dim0_qual;

    if (equalc(tok, "(")) {
        Token *next = tok->next;
        // Detect old-style (K&R) parameter lists: identifier-only params.
        // If the first token inside () is an identifier (not a type name)
        // followed by ) or ,, leave it for the caller (parse/declaration)
        // to handle, so K&R function definitions are not mis-parsed here.
        if (next->kind == TK_IDENT && !is_typename(next) &&
            (equalc(next->next, ")") || equalc(next->next, ","))) {
            *rest = tok;
            return ty;
        }
        return declarator_params(rest, tok->next, ty);
    }

    *rest = tok;
    return ty;
}

// GCC __attribute__((vector_size(N))): build a vector type as a TY_STRUCT of
// N/elem_size scalar element-members (element type `elem`), so all existing
// struct machinery (by-value pass/return, copy, brace init, compound literals,
// ABI classification) applies unchanged. Vectors additionally allow subscript
// (handled in postfix) and, unlike arrays, are first-class by-value values.
// align is the natural vector alignment (== total size).
static Type *make_vector_type(Type *elem, int total_size) {
    if (!elem || elem->size <= 0 || (!is_integer(elem) && !is_flonum(elem) && elem->kind != TY_PTR))
        error("vector_size applied to non-scalar type");
    if (total_size <= 0 || total_size % (int)elem->size != 0)
        error("vector_size %d is not a multiple of element size %d", total_size, (int)elem->size);
    int n = total_size / (int)elem->size;
    Type *ty = arena_alloc(sizeof(Type));
    ty->kind = TY_STRUCT;
    ty->is_vector = true;
    ty->base = elem;
    ty->size = total_size;
    ty->align = total_size;
    Member head = {0};
    Member *cur = &head;
    for (int i = 0; i < n; i++) {
        Member *m = arena_alloc(sizeof(Member));
        m->ty = elem;
        char *nm = format("__v%d", i);
        m->name = str_intern(nm, strlen(nm));
        m->offset = i * (int)elem->size;
        cur = cur->next = m;
    }
    ty->members = head.next;
    ty->has_body = true; // synthetic type, always complete
    return ty;
}

// Non-static entry point for the same builder, used by type.c's
// __builtin_ia32_* return-type classifier (ia32_builtin_ret) so the
// intrinsic-builtin return types share the header typedefs' exact Type
// construction path (same member layout, same alignment).
Type *rcc_make_vector_type(Type *elem, int total_size) {
    return make_vector_type(elem, total_size);
}

// Apply a pending GCC __attribute__((mode(...))) to `ty`, resetting the
// global pending_mode flag. Must be called from every declarator() exit
// path that can have consumed a mode() attribute -- read_type_attrs() can
// set pending_mode from EITHER a leading position (`int
// __attribute__((mode(DI))) x;`, checked at declarator()'s entry) OR a
// trailing one right after the identifier (`typedef unsigned long
// mp_word __attribute__((mode(TI)));`, GCC's own and libtommath's actual
// convention). Only the entry-point call site used to consume it: a
// trailing-position mode() was parsed (setting pending_mode as a side
// effect) but never applied to `ty` in THIS declarator() call, so the
// flag leaked, global and unreset, into whatever declarator() ran next --
// silently resizing/retyping an unrelated, unattributed declaration
// instead of the one that actually carried the attribute.
static Type *apply_pending_mode(Type *ty) {
    if (!pending_mode) return ty;
    if (pending_mode == 5) {
        // TI (128-bit): unlike QI/HI/SI/DI, this can't be handled by just
        // resizing a copy of `ty` -- codegen dispatches 128-bit
        // arithmetic on Type::kind == TY_INT128, not on size alone, so a
        // copy that merely grew to size=16 while keeping e.g. TY_LONG's
        // kind would still get 8-byte instructions/registers, silently
        // truncating every operation instead of erroring. Route through
        // the real, already-correct ty_int128/ty_uint128 singletons
        // instead, preserving the base type's own signedness (e.g.
        // libtommath's `typedef unsigned long mp_word
        // __attribute__((mode(TI)));` must become unsigned __int128, not
        // signed).
        ty = ty->is_unsigned ? ty_uint128 : ty_int128;
    } else {
        ty = copy_type(ty);
        int sizes[] = {0, 1, 2, 4, 8};
        ty->size = sizes[pending_mode];
        ty->align = ty->size;
    }
    pending_mode = 0;
    return ty;
}

static Type *declarator(Token **rest, Token *tok, Type *ty, char **name, VarAttr *attr) {
    int decl_align = 0;
    tok = read_type_attrs(tok, &decl_align, attr);
    ty = apply_pending_mode(ty);
    while (equalc(tok, "*")) {
        ty = pointer_to(ty);
        tok = tok->next;
        Token *attr_start = tok;
        VarAttr ptr_attr = {};
        tok = read_type_attrs(tok, &decl_align, &ptr_attr);
        if (ptr_attr.is_weak)
            pending_weak = true;
        if (ptr_attr.has_visibility) {
            pending_visibility_set = true;
            pending_visibility = ptr_attr.visibility;
        }
        // GNU allows function attributes after a pointer star in the
        // declarator (`extern __inline void * __attribute__((__gnu_inline__))
        // fn(void)` — gcc's lwpintrin.h). Merge the function-relevant ones
        // into the caller's attr instead of dropping them, or the
        // gnu-inline wrappers fail the eliminate-unused pass and their
        // bodies get codegen'd.
        if (attr) {
            if (ptr_attr.is_gnu_inline) attr->is_gnu_inline = true;
            if (ptr_attr.is_always_inline) attr->is_always_inline = true;
            if (ptr_attr.is_inline) attr->is_inline = true;
        }
        if (tok != attr_start &&
            (attr_start->kw == ID__ALIGNAS ||
             (attr_start->kw == ID_ALIGNAS && opt_std_version &&
              strcmp(opt_std_version, "202311L") >= 0)))
            error_tok(attr_start, "'_Alignas' cannot be specified for a pointer");
        unsigned char pq = collect_type_quals(&tok, tok);
        if (pq) {
            ty = copy_type(ty);
            ty->qual |= pq;
        }
    }

    Token *inner = tok->next;
    while (equalc(inner, "__cdecl") || equalc(inner, "__stdcall") || equalc(inner, "__fastcall") ||
           equalc(inner, "__thiscall") || equalc(inner, "__vectorcall"))
        inner = inner->next;
    inner = skip_attributes(inner);
    // C11 6.7.6p3 disambiguation: `(` immediately followed by `)` or a
    // type-specifier (e.g. abstract `void()` as a parameter type, or a
    // plain `int foo(void)` declarator) is a function-declarator suffix,
    // not a grouped/nested inner declarator - only `(*`, `(ident`, or
    // another `(` genuinely nests. Without this, `void (*f)(void())`
    // mis-parsed f's own inner parameter `void()` as bare `void` (the
    // `()` silently discarded instead of building a TY_FUNC), corrupting
    // call-argument classification against it (GH: torture test 921215-1).
    if (equalc(tok, "(") && !equalc(inner, ")") && !is_typename(inner)) {
        Token *start = tok->next;
        // Find the matching ) for the initial (
        Token *after_paren = start;
        int depth = 1;
        while (depth > 0 && after_paren->kind != TK_EOF) {
            if (equalc(after_paren, "(")) depth++;
            else if (equalc(after_paren, ")"))
                depth--;
            after_paren = after_paren->next;
        }
        // An unclosed '(' — the loop above ran out of tokens (hit TK_EOF)
        // before depth returned to 0. `start` (this declarator's would-be
        // inner token, passed to the recursive declarator() call below)
        // can then legitimately BE that terminal TK_EOF token, whose own
        // `->next` is NULL (the lexer's genuine end-of-list sentinel,
        // never explicitly linked further) -- the recursive call's own
        // `tok->next` dereference two lines into declarator() then reads
        // that NULL, and the resulting NULL token flows unchecked into
        // skip_attributes()/read_type_attrs(), which dereferences
        // `tok->kw` and segfaults. A lone '(' with nothing to close it is
        // a genuine syntax error; diagnose it here instead of recursing
        // into a token stream that has already run out.
        if (depth > 0)
            error_tok(tok, "expected ')'");
        tok = after_paren;
        Type *suffixed = type_suffix(&tok, tok, ty, NULL);
        *rest = tok;
        return declarator(&tok, start, suffixed, name, attr);
    }

    // Skip calling convention keywords, attributes, and pointer declarators before the identifier
    for (;;) {
        while (equalc(tok, "__cdecl") || equalc(tok, "__stdcall") || equalc(tok, "__fastcall") ||
               equalc(tok, "__thiscall") || equalc(tok, "__vectorcall"))
            tok = tok->next;
        Token *cur = tok;
        tok = read_type_attrs(tok, &decl_align, NULL);
        if (ty->kind == TY_PTR && tok != cur &&
            (cur->kw == ID__ALIGNAS ||
             (cur->kw == ID_ALIGNAS && opt_std_version &&
              strcmp(opt_std_version, "202311L") >= 0)))
            error_tok(cur, "'_Alignas' cannot be specified for a pointer");
        if (equalc(tok, "*")) {
            ty = pointer_to(ty);
            tok = tok->next;
            continue;
        }
        break;
    }

    // asm/__asm__/__asm is not a declarator identifier — let stmt() handle inline asm
    if (equalc(tok, "asm") || equalc(tok, "__asm__") || equalc(tok, "__asm")) {
        if (name) *name = NULL;
        *rest = tok;
        return ty;
    }

    if (tok->kind != TK_IDENT) {
        if (name)
            *name = NULL;
        ty = apply_pending_mode(ty);
        ty = type_suffix(rest, tok, ty, NULL);
        if (pending_vector_size) {
            ty = make_vector_type(ty, pending_vector_size);
            pending_vector_size = 0;
        }
        return apply_type_align(ty, decl_align);
    }

    char *decl_name = tok->name;
    if (name)
        *name = decl_name;
    tok = tok->next;
    VarAttr trail_attr = {};
    tok = read_type_attrs(tok, &decl_align, &trail_attr);
    ty = apply_pending_mode(ty);
    if (trail_attr.is_transparent_union)
        pending_transparent_union = true;
    // A trailing __attribute__((weak)) right after the identifier (e.g.
    // `int x __attribute__((weak));`) was parsed into trail_attr but
    // never propagated anywhere -- only the pointer-attribute case just
    // above (`int *p __attribute__((weak))`) set pending_weak. Both
    // placements need the same result: this is how the Plan9/Go-
    // toolchain AUTOLIB() idiom (`int __p9l_autolib_x
    // __attribute__((weak));`, u.h) and ordinary weak global variables
    // declare weak linkage.
    if (trail_attr.is_weak)
        pending_weak = true;
    if (trail_attr.has_visibility) {
        pending_visibility_set = true;
        pending_visibility = trail_attr.visibility;
    }
    // Propagate a trailing __attribute__((packed)) (e.g. busybox's own
    // `uint32_t crc32 __attribute__((packed));` idiom, tightening one
    // field's alignment without packing the whole struct) back to the
    // caller so struct-member layout can apply it -- unlike is_weak this
    // has no global "pending" flag to route through, and previously had
    // nowhere else to go at all: trail_attr was entirely local and this
    // information was silently dropped.
    if (attr && trail_attr.is_packed)
        attr->is_packed = true;
    ty = type_suffix(rest, tok, ty, decl_name);
    if (pending_vector_size) {
        ty = make_vector_type(ty, pending_vector_size);
        pending_vector_size = 0;
    }
    return apply_type_align(ty, decl_align);
}

static Type *enum_specifier(Token **rest, Token *tok) {
    tok = skip(tok, "enum");
    // C23: attributes allowed between enum keyword and tag name
    bool c23_tag_attrs = equalc(tok, "[") && tok->next &&
        equalc(tok->next, "[") && tok->ptr + tok->len == tok->next->ptr;
    // GCC also honors `packed` here (between `enum` and the tag name,
    // e.g. `enum __attribute__((packed)) E { ... };`) -- verified
    // against real GCC: distinct from the C23 `[[...]]` attribute-list
    // position checked above, this is the classic GNU
    // `__attribute__((...))` spelling and narrows the underlying type
    // exactly like the trailing `enum { ... } __attribute__((packed))`
    // form below (see `is_packed` at the end of this function).
    VarAttr leading_attr = {0};
    tok = read_type_attrs(tok, NULL, &leading_attr);
    bool leading_packed = leading_attr.is_packed;
    char *tag_name = NULL;
    if (tok->kind == TK_IDENT) {
        tag_name = tok->name;
        tok = tok->next;
    }
    if (c23_tag_attrs && tag_name && !equalc(tok, "{") && !equalc(tok, ";") &&
        !equalc(tok, ":"))
        error_tok(tok, "expected '{' or ';' after attributes on enum tag");

    // C23: optional fixed underlying type — enum [tag] : type
    // Only consume ':' if what follows is a real type specifier, not a
    // _Generic association expression like `enum H: 1` where '1' is not a type.
    Type *fixed_underlying = NULL;
    if (equalc(tok, ":") && is_typename(tok->next)) {
        tok = tok->next;
        VarAttr underlying_attr = {0};
        fixed_underlying = declspec(&tok, tok, &underlying_attr);
    }

    if (!equalc(tok, "{")) {
        *rest = tok;
        // `enum tag : type;` is a (re)declaration: it may shadow an outer
        // tag with a different underlying type, so don't look up — register.
        if (tag_name && !fixed_underlying) {
            for (EnumTag *et = enum_tags; et; et = et->next)
                if (et->name == tag_name) {
                    Type *ret = arena_alloc(sizeof(Type));
                    *ret = *et->ty;
                    return ret;
                }
        }
        Type *ety = arena_alloc(sizeof(Type));
        *ety = fixed_underlying ? *fixed_underlying : *ty_int;
        ety->qual = 0; // qualifiers on the underlying type don't apply
        ety->is_enum = true;
        ety->is_enum_fixed = (fixed_underlying != NULL);
        ety->enum_id = ety;
        // Register EVERY tagged forward declaration -- fixed-underlying-type
        // ones (`enum tag : type;`) already did, so sizeof(enum tag) saw the
        // right size; a bare `enum tag;` (GNU/C23 opaque forward reference,
        // e.g. GNU make's makeint.h: `enum variable_origin;` ahead of a
        // prototype using it, completed later by variable.h's `enum
        // variable_origin { ... }`) did NOT, so the eventual completion at
        // the enum-body path below found no existing tag and minted an
        // UNRELATED enum_id -- the placeholder and the completed type then
        // looked like two different, incompatible enums to
        // types_compatible_p()/the redeclaration-conflict check, even
        // though C treats a forward reference completed later as one type.
        // Registering here lets the body-parsing path's `existing_ty`
        // lookup find and reuse this exact identity.
        if (tag_name) {
            EnumTag *et = arena_alloc(sizeof(EnumTag));
            et->name = tag_name;
            et->ty = ety;
            et->depth = current_block_depth;
            et->members_int = false; // fixed underlying: members get enum type
            et->next = enum_tags;
            enum_tags = et;
        }
        return ety;
    }

    tok = tok->next;
    int64_t val = 0;
    __int128 min_val = 0, max_val = 0;
    bool first = true;
    EnumConst *before_consts = enum_consts; // consts list head before this enum
    Type *prev_ty = NULL; // during-definition type of the previous enumerator
    bool any_outside_int = false;
    // C23 tag compatibility: a redefinition of an already-completed enum
    // reuses the existing enum type, and its enumerators carry that type
    // (when not representable as int) already during the redefinition.
    Type *existing_ty = NULL;
    bool existing_members_int = false;
    if (tag_name)
        for (EnumTag *et = enum_tags; et; et = et->next)
            if (et->name == tag_name && et->ty->is_enum) {
                // Only a same-scope redefinition reuses the completed type;
                // an inner-scope definition shadows the outer tag.
                if (et->depth == current_block_depth) {
                    existing_ty = et->ty;
                    existing_members_int = et->members_int;
                }
                break;
            }
    // C23 (N3030): an enum with a fixed underlying type is complete
    // immediately after the underlying-type specifier — before any
    // enumerator is parsed — and every enumerator constant has exactly
    // that type from the start (not a provisional int/expr type only
    // fixed up at the end). Build the completed type and register its
    // tag now so self-referential uses inside the body (typeof(),
    // sizeof(enum X), another enumerator's initializer) already see it.
    Type *fixed_ret = NULL;
    if (fixed_underlying) {
        if (existing_ty) {
            fixed_ret = existing_ty;
        } else {
            fixed_ret = arena_alloc(sizeof(Type));
            *fixed_ret = *fixed_underlying;
            fixed_ret->qual = 0; // qualifiers on a fixed underlying type don't apply
            fixed_ret->is_enum = true;
            fixed_ret->is_enum_fixed = true;
            fixed_ret->enum_id = fixed_ret;
            if (tag_name) {
                EnumTag *et = arena_alloc(sizeof(EnumTag));
                et->name = tag_name;
                et->ty = fixed_ret;
                et->depth = current_block_depth;
                et->members_int = false; // fixed underlying: members get enum type
                et->next = enum_tags;
                enum_tags = et;
            }
        }
    }
    while (!equalc(tok, "}")) {
        if (tok->kind != TK_IDENT)
            error_tok(tok, "expected enum constant");

        EnumConst *ec = arena_alloc(sizeof(EnumConst));
        ec->name = tok->name;
        tok = tok->next;
        // C23: attributes allowed after enum constant name
        tok = read_type_attrs(tok, NULL, NULL);

        bool explicit_val = false;
        Type *expr_ty = NULL;
        if (equalc(tok, "=")) {
            tok = tok->next;
            Node *node = conditional(&tok, tok);
            check_type(node);
            long long v = 0;
            if ((node->ty && node->ty->kind == TY_NULLPTR_T) ||
                !eval_const_expr(node, &v))
                error_tok(tok, "expected constant expression for enum value");
            val = v;
            explicit_val = true;
            expr_ty = node->ty;
        }

        ec->val = val++;
        if (fixed_underlying) {
            // C23: enumerators of a fixed-underlying-type enum always have
            // the enum type itself, regardless of whether their value fits
            // int — no int/expr-type ladder applies.
            ec->ty = fixed_ret;
        } else {
            // C23 6.7.2.2: during definition an enumerator has type int when its
            // value is representable as int; otherwise the type of its defining
            // expression, or (for an implicit prev+1 value) the previous
            // enumerator's type widened on overflow, preserving signedness.
            bool fits_int = ec->val >= INT32_MIN && ec->val <= INT32_MAX &&
                !(explicit_val && expr_ty && expr_ty->is_unsigned && expr_ty->size > 4 && (uint64_t)ec->val > INT32_MAX);
            if (explicit_val) {
                if (fits_int)
                    ec->ty = ty_int;
                else
                    ec->ty = (expr_ty && is_integer(expr_ty)) ? expr_ty : ty_llong;
            } else if (fits_int && (!prev_ty || !prev_ty->is_unsigned || (uint64_t)ec->val <= INT32_MAX)) {
                ec->ty = (prev_ty && prev_ty != ty_int) ? prev_ty : ty_int;
            } else if (prev_ty && prev_ty->is_unsigned) {
                // unsigned progression: uint -> unsigned long -> unsigned long long
                if (prev_ty->size <= 4 && (uint64_t)ec->val <= 0xFFFFFFFFULL)
                    ec->ty = prev_ty;
                else
                    ec->ty = prev_ty->size <= 4 ? ty_ulong : ty_ullong;
            } else {
                // signed progression: int -> long -> long long
                ec->ty = (prev_ty && prev_ty->size > 4) ? ty_llong : ty_long;
            }
            if (existing_ty)
                ec->ty = existing_members_int ? ty_int : existing_ty;
        }
        if (ec->ty != ty_int)
            any_outside_int = true;
        prev_ty = ec->ty;
        // Widen this enumerator's raw 64-bit value into its true
        // mathematical value under its OWN signedness before tracking
        // min/max — a huge unsigned value (e.g. -1ull == UINT64_MAX) must
        // never compare as "less than" a small positive one just because
        // its int64_t bit pattern looks negative.
        __int128 ec_val128 = ec->ty->is_unsigned
            ? (__int128)(unsigned __int128)(uint64_t)ec->val
            : (__int128)ec->val;
        if (first) {
            min_val = max_val = ec_val128;
            first = false;
        } else {
            if (ec_val128 < min_val) min_val = ec_val128;
            if (ec_val128 > max_val) max_val = ec_val128;
        }
        ec->next = enum_consts;
        enum_consts = ec;
        enum_htab_add(ec);

        if (!equalc(tok, "}"))
            tok = skip(tok, ",");
    }

    // GNU extension: enum { ... } __attribute__((packed)) narrows the
    // underlying type to the smallest integer type that fits the range,
    // instead of the C-standard "at least int". Peek past '}' for it.
    VarAttr trailing_attr = {0};
    Token *after_attrs = read_type_attrs(tok->next, NULL, &trailing_attr);
    bool is_packed = trailing_attr.is_packed || leading_packed;
    *rest = after_attrs;
    // C23: with a fixed underlying type, the enum uses exactly that type;
    // otherwise choose the narrowest integer type >= int that fits all
    // values — int/unsigned int, then long/unsigned long, then (only if
    // still too narrow) long long/unsigned long long — matching GCC's own
    // enum finalization (or the narrowest type period, when __packed
    // forces a tight layout).
    Type *ety;
    // ty_long/ty_ulong's actual size is platform-dependent (8 bytes on
    // LP64 Linux/macOS, 4 bytes on LLP64 Windows/mingw, where `long` has
    // the same range as `int`) - the "long" tier's bounds must reflect
    // that, not a hardcoded LP64 assumption, or a value needing genuine
    // 64-bit range (e.g. LLONG_MIN) wrongly satisfies an INT64_MAX-based
    // check on a platform where `long` itself is only 32 bits, picking a
    // 4-byte enum type that then truncates every value outside int range.
    if (fixed_underlying) {
        ety = fixed_underlying;
    } else if (min_val >= 0) {
        uint64_t ulong_max = ty_ulong->size == 4 ? 0xFFFFFFFFULL : UINT64_MAX;
        if (is_packed && max_val <= 0xFF)
            ety = ty_uchar;
        else if (is_packed && max_val <= 0xFFFF)
            ety = ty_ushort;
        else if (max_val <= 0xFFFFFFFFLL)
            ety = ty_uint;
        else if (max_val <= (__int128)ulong_max)
            ety = ty_ulong;
        else
            ety = ty_ullong;
    } else {
        int64_t long_min = ty_long->size == 4 ? -2147483648LL : INT64_MIN;
        int64_t long_max = ty_long->size == 4 ? 2147483647LL : INT64_MAX;
        if (is_packed && min_val >= -128 && max_val <= 127)
            ety = ty_char;
        else if (is_packed && min_val >= -32768 && max_val <= 32767)
            ety = ty_short;
        else if (min_val >= -2147483648LL && max_val <= 2147483647LL)
            ety = ty_int;
        else if (min_val >= (__int128)long_min && max_val <= (__int128)long_max)
            ety = ty_long;
        else
            ety = ty_llong;
    }
    Type *ret;
    if (fixed_underlying) {
        ret = fixed_ret; // already completed before the body was parsed
    } else if (existing_ty) {
        ret = existing_ty; // redefinition: keep the completed type's identity
    } else {
        ret = arena_alloc(sizeof(Type));
        *ret = *ety;
        ret->qual = 0; // qualifiers on a fixed underlying type don't apply
        ret->is_enum = true;
        ret->enum_id = ret;
    }
    // C23 6.7.2.2: upon completion the enumerators get the enumerated type,
    // unless all values are representable as int (then they keep type int).
    // With a fixed underlying type they already have the enum type.
    if (fixed_underlying || any_outside_int)
        for (EnumConst *ec = enum_consts; ec && ec != before_consts; ec = ec->next)
            ec->ty = ret;
    // Push tag so subsequent references find the correct type (already
    // done up front above for the fixed-underlying-type case).
    if (tag_name && !fixed_underlying) {
        EnumTag *et = arena_alloc(sizeof(EnumTag));
        et->name = tag_name;
        et->ty = ret;
        et->depth = current_block_depth;
        et->members_int = !any_outside_int;
        et->next = enum_tags;
        enum_tags = et;
    }
    return ret;
}

// Build a Node expr computing align_to(x, align) at runtime:
// (x + (align-1)) & ~(align-1). Returns x unchanged if align <= 1.
static Node *vla_align_to_runtime(Node *x, int64_t align, Token *tok) {
    if (align <= 1)
        return x;
    int64_t mask = align - 1;
    Node *padded = new_binary(ND_ADD, x, new_num(mask, tok), tok);
    check_type(padded);
    Node *mask_n = new_num(~mask, tok);
    check_type(mask_n);
    Node *r = new_binary(ND_BITAND, padded, mask_n, tok);
    check_type(r);
    return r;
}

// Freeze a runtime size/offset expression into a hidden local variable at
// struct-definition time (so later changes to the VLA dimension variables
// don't retroactively change the layout already computed), chaining the
// capture statement onto pending_vla_struct_capture. Returns a fresh
// reference to the captured value.
static Node *vla_capture(Node *expr, Token *tok) {
    LVar *cap = new_var("", ty_long, true);
    Node *cap_lhs = new_var_node(cap, tok);
    Node *assign = new_binary(ND_ASSIGN, cap_lhs, expr, tok);
    check_type(assign);
    Node *stmt = new_unary(ND_EXPR_STMT, assign, tok);
    if (pending_vla_struct_capture) {
        Node *p = pending_vla_struct_capture;
        while (p->next)
            p = p->next;
        p->next = stmt;
    } else {
        pending_vla_struct_capture = stmt;
    }
    return new_var_node(cap, tok);
}

// C11 6.7.6.2p5: the size expressions of a variably-modified type
// (a pointer chain ending in a VLA, e.g. `int (*)[++i]` from a
// declarator, typeof, or cast) are evaluated exactly once where the
// type appears. Freeze each dimension into a hidden local: the
// evaluation assigns are chained onto *pre (as ND_COMMA), and the
// returned type references the frozen values, so a later sizeof(*p)
// reads them without re-running side effects.
static Type *vla_freeze_dims(Type *ty, Node **pre, Token *tok) {
    if (!ty)
        return ty;
    if (ty->kind == TY_FUNC) {
        // A pointer chain may pass through a function type on its way to
        // a VM array, e.g. `int (*(*)(void))[++l]` (pointer to function
        // returning pointer to VLA). Parameter types aren't reachable
        // from a cast/typeof's abstract-declarator, so only the return
        // type needs walking.
        Type *nr = vla_freeze_dims(ty->return_ty, pre, tok);
        if (nr == ty->return_ty)
            return ty;
        ty = copy_type(ty);
        ty->return_ty = nr;
        return ty;
    }
    if (ty->kind != TY_PTR && ty->kind != TY_VLA)
        return ty;
    Type *nb = vla_freeze_dims(ty->base, pre, tok);
    Node *cap_ref = NULL;
    if (ty->kind == TY_VLA && ty->vla_len_expr) {
        LVar *cap = new_var("", ty_long, true);
        Node *assign = new_binary(ND_ASSIGN, new_var_node(cap, tok), ty->vla_len_expr, tok);
        check_type(assign);
        if (*pre) {
            *pre = new_binary(ND_COMMA, *pre, assign, tok);
            check_type(*pre);
        } else {
            *pre = assign;
        }
        cap_ref = new_var_node(cap, tok);
        check_type(cap_ref);
    }
    if (nb != ty->base || cap_ref) {
        ty = copy_type(ty);
        ty->base = nb;
        if (cap_ref)
            ty->vla_len_expr = cap_ref;
    }
    return ty;
}

// Is ty variably modified (a VLA, or pointer/array chain reaching one)?
static bool is_vm_type(Type *ty) {
    for (; ty; ty = ty->base) {
        if (ty->kind == TY_VLA)
            return true;
        if (ty->kind != TY_PTR && ty->kind != TY_ARRAY)
            break;
    }
    return false;
}

// C11 6.7.2.4: typeof with a variably-modified expression operand
// evaluates the operand. Queue the evaluation onto
// pending_vla_struct_capture; declaration() flushes it as a statement
// ahead of the declarator it belongs to.
static void queue_vm_typeof_eval(Node *node, Token *tok) {
    if (!parser_current_fn || !is_vm_type(node->ty))
        return;
    Node *ev = new_unary(ND_EXPR_STMT, node, tok);
    if (pending_vla_struct_capture) {
        Node *p = pending_vla_struct_capture;
        while (p->next)
            p = p->next;
        p->next = ev;
    } else {
        pending_vla_struct_capture = ev;
    }
}

// ++/-- on a pointer to a VLA must advance by the runtime element size.
// Codegen's INC/DEC nodes use a compile-time delta, so desugar to
// assignment form and let ND_ADD's runtime VLA scaling handle it.
// Returns NULL when lhs is not a pointer-to-VLA (caller keeps INC/DEC).
static Node *vla_ptr_incdec(Node *lhs, bool is_inc, bool is_post, Token *tok) {
    check_type(lhs);
    if (!lhs->ty || lhs->ty->kind != TY_PTR || !lhs->ty->base || lhs->ty->base->kind != TY_VLA)
        return NULL;
    Node *rd = arena_alloc(sizeof(Node));
    *rd = *lhs;
    Node *add = new_binary(is_inc ? ND_ADD : ND_SUB, rd, new_num(1, tok), tok);
    check_type(add);
    Node *upd = new_binary(ND_ASSIGN, lhs, add, tok);
    check_type(upd);
    if (!is_post)
        return upd;
    // p++: (tmp = p, p = p + 1, tmp)
    LVar *tmp = new_var("", lhs->ty, true);
    Node *rd2 = arena_alloc(sizeof(Node));
    *rd2 = *lhs;
    Node *save = new_binary(ND_ASSIGN, new_var_node(tmp, tok), rd2, tok);
    check_type(save);
    Node *ret = new_var_node(tmp, tok);
    check_type(ret);
    Node *inner = new_binary(ND_COMMA, upd, ret, tok);
    check_type(inner);
    Node *seq = new_binary(ND_COMMA, save, inner, tok);
    check_type(seq);
    return seq;
}

// Does member m expose name n (directly or through an anonymous
// struct/union member)? Names are interned: pointer compare suffices.
static bool member_exposes(Member *m, char *n) {
    if (m->name)
        return m->name == n;
    if (m->ty && (m->ty->kind == TY_STRUCT || m->ty->kind == TY_UNION))
        for (Member *im = m->ty->members; im; im = im->next)
            if (member_exposes(im, n))
                return true;
    return false;
}

// C11 6.7.2.1: member names (including those reachable through anonymous
// members) must be unique within the enclosing struct/union.
static void check_duplicate_member(Member *list, Member *newm, Token *tok) {
    if (newm->tok)
        tok = newm->tok; // point at the member's own declaration line
    if (newm->name) {
        for (Member *m = list; m; m = m->next)
            if (member_exposes(m, newm->name))
                error_tok(tok, "duplicate member '%s'", newm->name);
    } else if (newm->ty &&
               (newm->ty->kind == TY_STRUCT || newm->ty->kind == TY_UNION)) {
        for (Member *im = newm->ty->members; im; im = im->next)
            check_duplicate_member(list, im, tok);
    }
}

// Forward decl: type_equal() (member-type comparison for identical-
// redefinition detection below) is defined much later, alongside the
// rest of _Generic's compatible-type machinery.
static bool type_equal(Type *a, Type *b);
static bool types_compatible_p(Type *a, Type *b);

// True if `a` and `b` are structurally identical struct/union bodies —
// same member count, each pair same name/type/offset/bitfield packing,
// same overall size/align. Used only to recognize a byte-for-byte
// identical same-scope tag redefinition (see struct_or_union_specifier's
// `redef_of` handling) as the SAME type, matching real GCC.
static bool struct_bodies_identical(Type *a, Type *b) {
    if (a->size != b->size || a->align != b->align)
        return false;
    Member *ma = a->members, *mb = b->members;
    for (; ma && mb; ma = ma->next, mb = mb->next) {
        if (ma->name != mb->name)
            return false;
        if (ma->offset != mb->offset || ma->bit_width != mb->bit_width)
            return false;
        if (ma->bit_width && ma->bit_offset != mb->bit_offset)
            return false;
        if (!type_equal(ma->ty, mb->ty))
            return false;
    }
    return !ma && !mb;
}

static Type *struct_or_union_specifier(Token **rest, Token *tok, bool is_union) {
    tok = tok->next;
    int struct_attr_align = 0;
    VarAttr struct_attr = {};
    // C23 [[]] attributes in this position are only valid when the type
    // contents are being defined, or in the lone "struct attr id;" form.
    bool c23_tag_attrs = equalc(tok, "[") && tok->next &&
        equalc(tok->next, "[") && tok->ptr + tok->len == tok->next->ptr;
    tok = read_type_attrs(tok, &struct_attr_align, &struct_attr);
    char *type_cleanup = pending_cleanup_func;
    pending_cleanup_func = NULL;
    pending_cleanup_tok = NULL;

    Token *tag_tok = NULL;
    if (tok->kind == TK_IDENT) {
        tag_tok = tok;
        tok = tok->next;
    }
    if (c23_tag_attrs && tag_tok && !equalc(tok, "{") && !equalc(tok, ";"))
        error_tok(tok, "expected '{' or ';' after attributes on struct or union tag");

    Type *ty = NULL;
    Type *redef_of = NULL; // set only for "redefine an already-complete tag" (see below)
    if (tag_tok) {
        TagScope *tag = find_tag(tag_tok);
        if (tag && !equalc(tok, "{")) {
            // Forward-reference use: return existing type
            ty = tag->ty;
        } else if (tag && equalc(tok, "{") && !tag->ty->has_body) {
            // Completing a forward-declared (incomplete) type: reuse existing Type object
            // so all pointers to it (typedefs, etc.) see the completed definition.
            ty = tag->ty;
        } else if (tag && equalc(tok, "{") && tag->ty->has_body && tag->depth == current_block_depth) {
            // Redefining an ALREADY-complete tag with a fresh body — e.g.
            // noplate's `#define span(T) struct CONCAT(span_, T) { ... }`
            // idiom, which re-emits the full `struct span_float { ... }`
            // body at every use site, relying on GCC's real (confirmed)
            // behavior of silently treating a byte-for-byte identical
            // redefinition as the SAME type rather than a distinct one —
            // load-bearing for `_Generic`/`__builtin_types_compatible_p`,
            // which key struct identity on this exact Type pointer
            // (type_equal()'s `a->members == b->members`). Registering the
            // new Type here immediately would make every later `span(T)*`
            // reference a *different* pointer identity from the one
            // captured at `vf`'s own declaration, so `_Generic` would
            // never match. Parse this body into its own fresh Type first
            // (below), then compare structurally against `tag->ty` once
            // complete; push_tag() only runs, deferred, if they differ.
            // Gated on `tag->depth == current_block_depth` (mirrors
            // EnumTag's identical convention): a NESTED-scope tag of the
            // same name is ordinary shadowing (always a fresh, distinct
            // type per C11/C23 alike, initially incomplete during its own
            // body parse) and must fall through to the plain "New
            // definition" branch below instead.
            ty = arena_alloc(sizeof(Type));
            ty->kind = is_union ? TY_UNION : TY_STRUCT;
            ty->size = 0;
            ty->align = 1;
            ty->bitfield_mode = struct_attr.bitfield_mode;
            ty->struct_id = ty;
            redef_of = tag->ty;
        } else {
            // New definition (possibly shadowing an outer-scope tag)
            ty = arena_alloc(sizeof(Type));
            ty->kind = is_union ? TY_UNION : TY_STRUCT;
            ty->size = 0;
            ty->align = 1;
            ty->bitfield_mode = struct_attr.bitfield_mode;
            ty->struct_id = ty;
            push_tag(tag_tok->name, ty);
        }
    } else {
        ty = arena_alloc(sizeof(Type));
        ty->kind = is_union ? TY_UNION : TY_STRUCT;
        ty->size = 0;
        ty->align = 1;
        ty->bitfield_mode = struct_attr.bitfield_mode;
    }

    if (struct_attr.bitfield_mode)
        ty->bitfield_mode = struct_attr.bitfield_mode;

    if (!equalc(tok, "{")) {
        *rest = tok;
        return ty;
    }

    tok = tok->next;
    Member head = {};
    Member *cur = &head;
    int64_t offset = 0;
    int64_t max_size = 0;
    int max_align = 1;
    // Once non-NULL, holds the running runtime byte offset for the next
    // member (set after the first variable-size/VLA member is seen).
    Node *vla_off_acc = NULL;
    int bit_pos = 0; // current bit position within the struct (for bitfield packing)
    int struct_pack = pack_align; // capture #pragma pack value at struct start
    if (struct_attr.is_packed && struct_pack == 0) struct_pack = 1;
    // A packed attribute can also trail the whole declaration —
    // `struct S { members... } __attribute__((packed));` — rather than
    // lead it (`struct __attribute__((packed)) S {...}`, already handled
    // via struct_attr above). GCC packs the *entire* type this way,
    // including every member's offset, but that decision has to be made
    // before laying out a single member below, while the attribute
    // itself is only visible after all of them. Cheaply look ahead past
    // this body's matching closing brace (plain brace-depth counting,
    // not real parsing) so struct_pack already reflects it once the
    // member loop starts; the same tokens get parsed again for real once
    // control actually reaches that closing brace.
    if (struct_pack == 0) {
        int depth = 1;
        Token *scan = tok;
        while (scan->kind != TK_EOF && depth > 0) {
            if (equalc(scan, "{")) depth++;
            else if (equalc(scan, "}"))
                depth--;
            if (depth > 0) scan = scan->next;
        }
        if (depth == 0) {
            int la_align = 0;
            VarAttr la_attr = {0};
            read_type_attrs(scan->next, &la_align, &la_attr);
            if (la_attr.is_packed) struct_pack = 1;
        }
    }
    bool use_ms_bitfields = false;
    if (!is_union) {
        if (ty->bitfield_mode == BF_MODE_MS)
            use_ms_bitfields = true;
        else if (ty->bitfield_mode == BF_MODE_GCC)
            use_ms_bitfields = false;
        else
            use_ms_bitfields = opt_ms_bitfields;
    }
    int ms_run_base = 0;
    TypeKind ms_prev_kind = TY_FUNC;
    int ms_prev_bit_width = 0;

    // C11 6.7.5's alignas-not-in-type-name restriction applies to the
    // OUTER type-name's own declarator syntax, not to member
    // declarations nested inside a struct/union body written as that
    // type-name's specifier (e.g. `_Alignof(union { alignas(16) int
    // i; })`): each member declaration is an ordinary declaration in
    // its own right, where alignas is always allowed. Suspend
    // in_type_name for the whole body so nested declspec()/alignas
    // parsing isn't wrongly rejected just because the enclosing
    // struct/union specifier is itself being parsed as a type-name.
    bool _saved_in_type_name_su = in_type_name;
    in_type_name = false;
    while (!equalc(tok, "}")) {
        // A trailing _Pragma(...)/__attribute__/[[...]] with nothing but
        // the closing brace after it (e.g. glib's own
        // G_GNUC_END_IGNORE_DEPRECATIONS right before a struct's final
        // "};") must not be mistaken for the start of one more member
        // declaration -- declspec() would otherwise consume it, find no
        // real type keyword left, silently fall back to implicit int,
        // and hand the following "}" to declarator() as if it were a
        // member name. Peek (non-destructively) past any leading attrs/
        // pragmas; only actually consume them here if nothing but "}"
        // follows. A *real* member's own leading attribute (e.g.
        // "alignas(128) int x;") must reach declspec() untouched, so its
        // alignment is recorded via that call's own &attr_align/&attr
        // rather than silently discarded by a throwaway skip here.
        if (equalc(skip_attributes(tok), "}")) {
            tok = skip_attributes(tok);
            break;
        }
        VarAttr attr = {};
        pending_constructor = false;
        pending_destructor = false;
        pending_asm_name = NULL;
        pending_alias_target = NULL;
        pending_section_name = NULL;
        // C11 _Static_assert / C23 static_assert inside struct/union body
        if (equalc(tok, "_Static_assert") || equalc(tok, "static_assert")) {
            Token *st = tok;
            tok = skip(tok->next, "(");
            Node *cond = conditional(&tok, tok);
            check_type(cond);
            if (cond->ty && !is_integer(cond->ty))
                error_tok(cond->tok, "static_assert condition is not an integer");
            // C11 6.6p6: floating operands only as immediate cast operands
            if (cond->kind == ND_CAST && cond->lhs && cond->lhs->ty &&
                is_flonum(cond->lhs->ty) && cond->lhs->kind != ND_FNUM)
                warn_tok(cond->tok,
                         "static_assert condition is not an integer constant expression");
            long long v = 0;
            if (!eval_const_expr(cond, &v))
                error_tok(cond->tok, "static_assert condition must be constant");
            char *msg = "static_assert failed";
            if (equalc(tok, ",")) {
                tok = tok->next;
                if (tok->kind == TK_STR) {
                    msg = tok->str;
                    tok = tok->next;
                } else {
                    error_tok(tok, "expected string literal in static assertion");
                }
            }
            tok = skip(tok, ")");
            tok = skip(tok, ";");
            if (!v) error_tok(st, "%s", msg);
            continue;
        }
        // The preprocessor normalizes #pragma pack push/pop to
        // `# pragma pack (N)` with no stack semantics. GCC ignores such
        // directives when they appear inside a struct/union body (they
        // cannot change the layout of the enclosing type after parsing has
        // begun); rcc previously treated the leading `#` as a member
        // declaration and errored. Treat them as no-ops here.
        if (equalc(tok, "#") && equalc(tok->next, "pragma") &&
            (equalc(tok->next->next, "pack") || equalc(tok->next->next, "fenv"))) {
            tok = tok->next->next->next;
            if (equalc(tok, "(")) {
                tok = tok->next;
                if (tok->kind == TK_NUM)
                    tok = tok->next;
                if (equalc(tok, ")"))
                    tok = tok->next;
            }
            continue;
        }
        // C11 6.7.2.1p2: empty declaration (bare semicolon) is valid.
        // Kernels commonly produce these from empty __VA_ARGS__ in macros.
        if (equalc(tok, ";")) {
            tok = tok->next;
            continue;
        }
        Token *mdecl_start = tok;
        Type *base = declspec(&tok, tok, &attr);
        if (attr.is_typedef || attr.is_extern || attr.is_static)
            error_tok(tok, "invalid storage class in member declaration");
        if (attr.is_noreturn_std)
            error_tok(tok, "'_Noreturn' in member declaration");
        if (!base)
            error_tok(tok, "expected member type");
        if (equalc(tok, ";")) {
            // C11 6.7.2.1p13: an untagged struct/union specifier forms an
            // anonymous member. GNU extensions (rejected under -pedantic,
            // same as GCC), both also accepted as an unnamed field whose
            // members get promoted into the enclosing struct:
            // - A *tagged* struct/union with its own fresh body and no
            //   declarator, so it's still usable by name elsewhere — e.g.
            //   the kernel's socket_lock_t's
            //   `union { struct slock_owned { ... }; long combined; };`.
            // - A bare reference to a *previously completed* tagged
            //   struct/union, with no declarator and no fresh body of its
            //   own — e.g. `struct filename { struct __filename_head; ... };`.
            // A typedef name or a non-aggregate type still declares nothing.
            // Qualifiers (const/volatile/...) may precede or follow the
            // struct/union keyword (`const struct { ... };`), so locate it
            // by scanning rather than assuming it's the first token.
            Token *su_tok = NULL;
            for (Token *t = mdecl_start; t && t != tok; t = t->next)
                if (equalc(t, "struct") || equalc(t, "union")) {
                    su_tok = t;
                    break;
                }
            bool untagged_inline = su_tok && equalc(su_tok->next, "{");
            bool tag_has_fresh_body = su_tok && su_tok->next && su_tok->next->kind == TK_IDENT &&
                equalc(su_tok->next->next, "{");
            bool tagged_aggregate = !opt_pedantic && su_tok && !untagged_inline && !tag_has_fresh_body &&
                (base->kind == TY_STRUCT || base->kind == TY_UNION) && (base->members || base->size > 0);
            // Like tagged_aggregate, GCC only accepts a tagged-with-fresh-body
            // member as a GNU extension: rejected under -pedantic (torture's
            // c11-anon-struct-2.c relies on this — struct s4's `struct s {
            // int i; };` member must still error under -std=c11 -pedantic-errors).
            bool tag_fresh_body_ok = !opt_pedantic && tag_has_fresh_body;
            if (!untagged_inline && !tagged_aggregate && !tag_fresh_body_ok)
                error_tok(tok, "declaration does not declare anything");
            // codeql[cpp/commented-out-code]: prose summary using struct{...} shorthand, not actual code
            // Anonymous struct/union member: struct { ... }; or union { ... };
            if (base->kind == TY_STRUCT || base->kind == TY_UNION) {
                Member *mem = arena_alloc(sizeof(Member));
                mem->name = NULL;
                mem->bit_offset = 0;
                mem->bit_width = 0;
                mem->ty = base;
                if (is_union) {
                    mem->offset = 0;
                    if (max_size < base->size) max_size = base->size;
                    int a = base->align;
                    if (struct_pack > 0 && (struct_pack < a || a == 0)) a = struct_pack;
                    if (max_align < a) max_align = a;
                } else {
                    int a = base->align;
                    if (struct_pack > 0 && (struct_pack < a || a == 0)) a = struct_pack;
                    offset = align_to(offset, a);
                    mem->offset = offset;
                    offset += base->size;
                    bit_pos = offset * 8;
                    if (max_align < a) max_align = a;
                }
                mem->tok = mdecl_start;
                check_duplicate_member(head.next, mem, mdecl_start);
                cur = cur->next = mem;
            }
            tok = tok->next;
            continue;
        }

        for (;;) {
            char *name = NULL;
            // A trailing attribute after the member's OWN declarator (e.g.
            // `uint32_t crc32 __attribute__((packed));`, busybox's own
            // idiom for tightening layout field-by-field instead of
            // packing the whole struct) is distinct from the struct-level
            // trailing attribute after `}` handled below via struct_pack:
            // it overrides only THIS member's effective alignment, not
            // the type it names (mem_ty may be a shared typedef, e.g.
            // uint32_t, that must keep its normal alignment everywhere
            // else) nor later members. Confirmed against real gcc:
            // packing just the 4-byte fields this way removes exactly
            // the padding gcc would otherwise insert before each one,
            // with no effect on the plain uint16_t members around them.
            // declarator() itself consumes and merges it into mem_var_attr
            // (see its own trail_attr handling).
            int mem_align = 0;
            VarAttr mem_var_attr = {0};
            Type *mem_ty = declarator(&tok, tok, copy_type(base), &name, &mem_var_attr);
            tok = read_type_attrs(tok, &mem_align, &mem_var_attr);

            // Check for bitfield
            int bit_width = 0;
            if (equalc(tok, ":")) {
                tok = tok->next;
                Node *width_node = conditional(&tok, tok);
                check_type(width_node);
                long long w;
                if (width_node->ty && width_node->ty->kind == TY_NULLPTR_T)
                    error_tok(width_node->tok,
                              "bitfield width is not an integer constant expression");
                if (!eval_const_expr(width_node, &w))
                    error_tok(tok, "bitfield width must be a constant expression");
                bit_width = (int)w;
                if (bit_width < 0 || bit_width > mem_ty->size * 8)
                    error_tok(tok, "bitfield width out of range");
            }

            // Anonymous struct/union member (no name, no bitfield width, aggregate type)
            // e.g. "struct { u8 a, b; };" inside another struct/union
            if (!name && bit_width == 0 && (mem_ty->kind == TY_STRUCT || mem_ty->kind == TY_UNION)) {
                Member *mem = arena_alloc(sizeof(Member));
                mem->name = NULL;
                mem->bit_offset = 0;
                mem->bit_width = 0;
                mem->ty = mem_ty;
                if (is_union) {
                    mem->offset = 0;
                    if (max_size < mem_ty->size) max_size = mem_ty->size;
                    int a = mem_ty->align;
                    if (struct_pack > 0 && (struct_pack < a || a == 0)) a = struct_pack;
                    if (max_align < a) max_align = a;
                } else {
                    int a = mem_ty->align;
                    if (struct_pack > 0 && (struct_pack < a || a == 0)) a = struct_pack;
                    offset = align_to(offset, a);
                    mem->offset = offset;
                    offset += mem_ty->size;
                    bit_pos = offset * 8;
                    if (max_align < a) max_align = a;
                }
                mem->tok = mdecl_start;
                check_duplicate_member(head.next, mem, mdecl_start);
                cur = cur->next = mem;
                if (!equalc(tok, ",")) break;
                tok = tok->next;
                continue;
            }

            // Handle anonymous bitfield: "int : N" or "int : 0"
            // These don't create named members but affect layout
            if (!name && bit_width >= 0) {
                if (!is_union) {
                    int unit = mem_ty->size;
                    int unit_bits = unit * 8;
                    int align = mem_ty->align;
                    if (struct_pack > 0 && struct_pack < align)
                        align = struct_pack;
                    if (use_ms_bitfields) {
                        TypeKind kind = mem_ty->kind;
                        // codeql[cpp/constant-comparison]: `bit_width > 0`
                        // is not provably constant here — this branch runs
                        // for both a named/anonymous `:N` field and an
                        // anonymous `:0` field once use_ms_bitfields is set
                        // (unlike the non-MS `:0` path below, which is a
                        // separate `else if`). Kept identical to the
                        // named-bitfield copy below rather than
                        // special-cased, to avoid diverging MS bitfield
                        // layout logic between the two call sites.
                        bool new_run = bit_pos + bit_width > unit_bits ||
                            ((bit_width > 0) == (kind != ms_prev_kind));
                        if (new_run) {
                            offset = align_to(offset, align);
                            ms_run_base = offset;
                            bit_pos = 0;
                            if (bit_width > 0 || ms_prev_bit_width > 0)
                                offset += unit;
                        }
                        ms_prev_kind = kind;
                        ms_prev_bit_width = bit_width;
                        bit_pos += bit_width;
                        if (bit_width > 0 && align > max_align)
                            max_align = align;
                    } else if (bit_width == 0) {
                        // :0 always advances to the next T-aligned boundary
                        // (uses declared type size regardless of struct_pack)
                        int unit_base = (bit_pos / unit_bits) * unit;
                        if (bit_pos % unit_bits != 0)
                            unit_base += unit;
                        if (unit_base > offset) offset = unit_base;
                        bit_pos = unit_base * 8;
                    } else {
                        // :N — advance bit_pos by N bits within layout
                        if (struct_pack > 0) {
                            // Dense packing: just advance cursor
                            bit_pos += bit_width;
                        } else {
                            // Non-packed GCC rule: fit within T-aligned unit
                            int unit_base = (bit_pos / unit_bits) * unit;
                            int bit_off = bit_pos - unit_base * 8;
                            if (bit_off + bit_width > unit_bits) {
                                unit_base += unit;
                                bit_pos = unit_base * 8;
                                if (unit_base > offset) offset = unit_base;
                            }
                            bit_pos += bit_width;
                        }
                        int end_byte = (bit_pos + 7) / 8;
                        if (end_byte > offset) offset = end_byte;
                    }
                }
                if (!equalc(tok, ","))
                    break;
                tok = tok->next;
                continue;
            }

            if (!name)
                error_tok(tok, "expected member name");

            Member *mem = arena_alloc(sizeof(Member));
            mem->name = name;
            mem->tok = tok;
            check_duplicate_member(head.next, mem, tok);
            mem->bit_width = bit_width;
            mem->bf_load_size = 0;

            if (bit_width > 0) {
                int unit = mem_ty->size; // storage unit size in bytes
                int unit_bits = unit * 8;
                int align = mem_ty->align;
                if (struct_pack > 0 && struct_pack < align)
                    align = struct_pack;

                mem->ty = mem_ty;

                if (is_union) {
                    mem->offset = 0;
                    mem->bit_offset = 0;
                    if (max_size < unit) max_size = unit;
                } else if (use_ms_bitfields) {
                    TypeKind kind = mem_ty->kind;
                    // bit_width > 0 always holds here (line 3539's outer
                    // guard), unlike the anonymous-bitfield copy of this
                    // check above which also covers `:0`.
                    bool new_run = bit_pos + bit_width > unit_bits || (kind != ms_prev_kind);
                    if (new_run) {
                        offset = align_to(offset, align);
                        ms_run_base = offset;
                        bit_pos = 0;
                        if (bit_width > 0 || ms_prev_bit_width > 0)
                            offset += unit;
                    }
                    mem->offset = ms_run_base;
                    mem->bit_offset = bit_pos;
                    ms_prev_kind = kind;
                    ms_prev_bit_width = bit_width;
                    bit_pos += bit_width;
                    if (align > max_align)
                        max_align = align;
                } else if (struct_pack > 0) {
                    // Dense packing (#pragma pack): place at current bit cursor,
                    // but if the member has an explicit alignment attribute
                    // (align > natural size), enforce at least byte-alignment
                    // (limited by struct_pack, so e.g. aligned(16) in pack(1) → 1).
                    if (mem_ty->align > mem_ty->size) {
                        int eff = mem_ty->align < struct_pack ? mem_ty->align : struct_pack;
                        if (eff < 1) eff = 1;
                        int aligned_byte = align_to((bit_pos + 7) / 8, eff);
                        bit_pos = aligned_byte * 8;
                        if (aligned_byte > offset) offset = aligned_byte;
                    }
                    int byte_pos = bit_pos / 8;
                    int bit_off = bit_pos % 8;
                    mem->offset = byte_pos;
                    mem->bit_offset = bit_off;
                    // If field crosses its declared type boundary, record larger load size
                    int needed = (bit_off + bit_width + 7) / 8;
                    if (needed > unit) {
                        int ls = unit;
                        while (ls < needed) ls *= 2;
                        mem->bf_load_size = ls;
                    }
                    bit_pos += bit_width;
                    int end_byte = (bit_pos + 7) / 8;
                    if (end_byte > offset) offset = end_byte;
                } else {
                    // GCC T-aligned unit algorithm (non-packed):
                    // Find the T-aligned storage unit that contains bit_pos.
                    // If the bitfield fits within that unit, place it there.
                    // Otherwise advance to the next T-aligned unit.
                    // If the member type has explicit alignment > unit size,
                    // the bitfield must start at that alignment boundary.
                    int unit_base = (bit_pos / unit_bits) * unit;
                    int bit_off = bit_pos - unit_base * 8;
                    if (bit_off + bit_width > unit_bits) {
                        unit_base += unit;
                        bit_off = 0;
                        bit_pos = unit_base * 8;
                    }
                    // Enforce explicit member alignment (e.g. __attribute__((aligned(16))) char a:4)
                    // Only when alignment was explicitly set beyond the type's natural size.
                    if (mem_ty->align > unit) {
                        unit_base = align_to(unit_base, mem_ty->align);
                        bit_off = 0;
                        bit_pos = unit_base * 8;
                    }
                    mem->offset = unit_base;
                    mem->bit_offset = bit_off;
                    if (unit_base > offset) offset = unit_base;
                    bit_pos = unit_base * 8 + bit_off + bit_width;
                    int end_byte = (bit_pos + 7) / 8;
                    if (end_byte > offset) offset = end_byte;
                    if (max_align < mem_ty->align) max_align = mem_ty->align;
                }
                if (max_align < unit)
                    max_align = unit;
            } else {
                // Normal (non-bitfield) member
                mem->ty = mem_ty;
                mem->bit_offset = 0;
                if (is_union) {
                    mem->offset = 0;
                    if (max_size < mem_ty->size)
                        max_size = mem_ty->size;
                    if (max_align < mem_ty->align)
                        max_align = mem_ty->align;
                } else {
                    // For VLA members, mem_ty->align is a fixed placeholder (8), not the
                    // element type's real alignment (also where __attribute__((aligned(N)))
                    // written before the element type ends up). Use the base type's
                    // alignment instead so layout matches GCC.
                    int a = (mem_ty->kind == TY_VLA) ? mem_ty->base->align : mem_ty->align;
                    if (mem_var_attr.is_packed) a = 1;
                    if (mem_align > a) a = mem_align;
                    if (struct_pack > 0 && (struct_pack < a || a == 0))
                        a = struct_pack;
                    bool mem_is_vla = mem_ty->kind == TY_VLA && mem_ty->vla_len_expr;

                    if (!vla_off_acc) {
                        offset = align_to(offset, a);
                        mem->offset = offset;
                        if (mem_is_vla) {
                            // VLA struct member: capture size into a hidden lvar now
                            // (before any n++ can change the VLA dimension variable).
                            // offset of next member = fixed_prefix + len * base_size,
                            // rounded up to this member's own alignment.
                            Node *base_sz_n = new_num(mem_ty->base->size, tok);
                            check_type(base_sz_n);
                            Node *vla_sz = new_binary(ND_MUL, mem_ty->vla_len_expr, base_sz_n, tok);
                            check_type(vla_sz);
                            Node *full_sz;
                            if (offset > 0) {
                                Node *off_n = new_num(offset, tok);
                                check_type(off_n);
                                full_sz = new_binary(ND_ADD, off_n, vla_sz, tok);
                                check_type(full_sz);
                            } else {
                                full_sz = vla_sz;
                            }
                            full_sz = vla_align_to_runtime(full_sz, a, tok);
                            vla_off_acc = vla_capture(full_sz, tok);
                        } else {
                            offset += mem_ty->size;
                            bit_pos = offset * 8;
                        }
                    } else {
                        // A previous member had a variable size, so this member's
                        // offset is itself a runtime expression.
                        Node *aligned_off = vla_align_to_runtime(vla_off_acc, a, tok);
                        Node *off_cap = vla_capture(aligned_off, tok);
                        mem->offset = 0;
                        mem->offset_expr = off_cap;
                        Node *sz_expr;
                        if (mem_is_vla) {
                            Node *base_sz_n = new_num(mem_ty->base->size, tok);
                            check_type(base_sz_n);
                            sz_expr = new_binary(ND_MUL, mem_ty->vla_len_expr, base_sz_n, tok);
                            check_type(sz_expr);
                        } else {
                            sz_expr = new_num(mem_ty->size, tok);
                            check_type(sz_expr);
                        }
                        Node *new_off = new_binary(ND_ADD, new_var_node(off_cap->var, tok), sz_expr, tok);
                        check_type(new_off);
                        vla_off_acc = vla_capture(new_off, tok);
                    }
                    ms_prev_kind = TY_FUNC;
                    ms_prev_bit_width = 0;
                    if (max_align < a)
                        max_align = a;
                }
                if (struct_pack > 0 && ty->pack_align == 0)
                    ty->pack_align = struct_pack;
            }
            if (name)
                cur = cur->next = mem;

            // Consume GCC function specifiers like __cond_acquires(true, lock)
            while (tok->kind == TK_IDENT && tok->next && equalc(tok->next, "(")) {
                tok = tok->next;
                tok = skip(tok, "(");
                int pdepth = 1;
                while (pdepth > 0 && tok->kind != TK_EOF) {
                    if (equalc(tok, "(")) pdepth++;
                    else if (equalc(tok, ")"))
                        pdepth--;
                    tok = tok->next;
                }
                if (equalc(tok, ","))
                    tok = tok->next;
            }
            if (!equalc(tok, ","))
                break;
            tok = tok->next;
        }

        // GNU extension (gcc: "warning: no semicolon at end of struct or
        // union"): the trailing ';' after a struct/union's last member
        // declarator-list may be omitted when '}' unambiguously
        // terminates it instead of erroring outright.
        if (equalc(tok, "}"))
            warn_tok(tok, "no semicolon at end of struct or union");
        else
            tok = skip(tok, ";");
    }

    tok = skip(tok, "}");
    in_type_name = _saved_in_type_name_su;
    if (type_cleanup)
        ty->cleanup_func = type_cleanup;
    ty->members = head.next;
    ty->has_body = true;
    int final_align = max_align;
    if (struct_pack > 0 && struct_pack < max_align)
        final_align = struct_pack;
    if (struct_attr_align > final_align)
        final_align = struct_attr_align;
    // A trailing __attribute__((aligned(N))) on the struct/union's own
    // specifier (`struct S { ... } __attribute__((aligned(N)));`, no
    // declarator) widens the *type itself* — every array of this type
    // must keep each element aligned, so unlike a declaration-level
    // alignas on one object (see apply_type_align), the type's own size
    // must pad up to N too. Peek here, before this specifier's tokens are
    // handed back to declspec's generic (non-size-padding) attribute loop.
    int trailing_align = 0;
    VarAttr trailing_attr = {0};
    Token *after_trailing_attrs = read_type_attrs(tok, &trailing_align, &trailing_attr);
    if (trailing_align > final_align)
        final_align = trailing_align;
    tok = after_trailing_attrs;
    ty->align = final_align;
    if (vla_off_acc) {
        // VLA-containing struct: the real runtime size lives in vla_len_expr
        // (used by sizeof and struct-copy codegen). ty->size keeps a fixed
        // placeholder (matching TY_VLA's) so ABI size-classification (>8,
        // pass-by-memory) treats it like a large/variable aggregate.
        ty->vla_len_expr = vla_capture(vla_align_to_runtime(vla_off_acc, final_align, tok), tok);
        ty->size = 16;
    } else {
        ty->size = is_union ? align_to(max_size, final_align) : align_to(offset, final_align);
    }
    // Complete any qualified INCOMPLETE variants in lockstep with the tag
    // (see qualify_struct_type / Type.qual_variants): a `const struct S*`
    // parsed while S was still forward-declared must read the finished
    // type's members/size/alignment here, yet its qualifier must never
    // leak onto the canonical type -- mimalloc.h's `const mi_heap_t*`
    // uses while `struct mi_heap_s` is incomplete would otherwise
    // const-qualify every later `mi_heap_t` declaration (e.g. init.c's
    // `mi_heap_t _mi_heap_main`), whose member reads eval_const_expr()
    // then wrongly folds as constant.
    for (Type *v = ty->qual_variants; v;) {
        Type *next = v->qual_variants; // *v = *ty clobbers the list link
        unsigned char use_qual = v->use_qual;
        *v = *ty;
        v->use_qual = use_qual;
        v->qual = ty->qual | use_qual;
        v->qual_variants = next;
        v = next;
    }
    if (redef_of && struct_bodies_identical(redef_of, ty)) {
        // Byte-for-byte identical redefinition (e.g. noplate's `span(T)`
        // idiom): discard the freshly parsed Type and hand back the
        // ORIGINAL one, so every _Generic/type-compat check keyed on
        // pointer identity still matches across every use site.
        *rest = tok;
        return redef_of;
    }
    if (redef_of)
        // Genuinely conflicting redefinition of the same tag: register
        // the new (shadowing) definition now, deferred from the branch
        // above so a self-referencing member during parsing still saw
        // the OLD tag.
        push_tag(tag_tok->name, ty);
    *rest = tok;
    return ty;
}

// C11 6.7.3p9: "If the specification of an array type includes any type
// qualifiers, the element type is so-qualified, not the array type." This
// applies not just when an array is declared directly with a qualified
// element specifier (`const int a[]` already builds array-of(const int)
// naturally, since the qualifier attaches to `int` before type_suffix()
// wraps it) but also when a qualifier prefixes a typedef name that is
// ITSELF an array type (`typedef int T[]; const T c;` — the qualifier
// must still land on the element, not on the array as a whole, or the
// array would incorrectly decay to a const POINTER instead of a pointer
// to const element). Recurses through multi-dimensional arrays/VLAs to
// the innermost element type.
static Type *qualify_array_elem(Type *ty, unsigned char quals) {
    if (ty->kind == TY_ARRAY) {
        int64_t len = ty->base->size ? ty->size / ty->base->size : 0;
        return array_of(qualify_array_elem(ty->base, quals), len);
    }
    if (ty->kind == TY_VLA)
        return vla_of(qualify_array_elem(ty->base, quals), ty->vla_len_expr, ty->array_len);
    return qualify_struct_type(ty, quals);
}

static Type *declspec(Token **rest, Token *tok, VarAttr *attr) {
    Type *ty = NULL;
    bool is_signed = false;
    bool is_unsigned = false;
    bool is_short = false;
    int long_count = 0;
    bool is_int = false;
    bool is_char = false;
    bool is_float = false;
    bool is_double = false;
    bool is_bool = false;
    bool is_void = false;
    bool is_int128 = false;
    bool is_complex = false;
    bool is_bitint = false;
    int bitint_width = 0;
    int attr_align = 0;
    unsigned char quals = 0;
    memset(attr, 0, sizeof(*attr));

    bool has_auto_seen = false;
    for (;;) {
        Token *attr_tok = read_type_attrs(tok, &attr_align, attr);
        if (attr_tok != tok) {
            tok = attr_tok;
            continue;
        }

        if (equalc(tok, "typedef")) {
            if (attr->is_typedef)
                error_tok(tok, "duplicate 'typedef'");
            attr->is_typedef = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "extern")) {
            if (attr->is_extern)
                error_tok(tok, "duplicate 'extern'");
            attr->is_extern = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "static")) {
            if (attr->is_static)
                error_tok(tok, "duplicate 'static'");
            if (attr->is_register)
                error_tok(tok, "multiple storage classes in declaration specifiers");
            attr->is_static = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "constexpr") && opt_std_version && strcmp(opt_std_version, "202311L") >= 0) {
            attr->is_constexpr = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "__auto_type")) {
            attr->is_auto_type = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "inline") || equalc(tok, "__inline") || equalc(tok, "__inline__")) {
            attr->is_inline = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Noreturn")) {
            attr->is_noreturn = true;
            attr->is_noreturn_std = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "register")) {
            if (attr->is_register)
                error_tok(tok, "duplicate 'register'");
            if (attr->is_static)
                error_tok(tok, "multiple storage classes in declaration specifiers");
            if (attr->is_tls)
                error_tok(tok, "'register' used with 'thread_local'");
            attr->is_register = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "auto")) {
            attr->is_auto = true;
            has_auto_seen = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "__thread") || equalc(tok, "_Thread_local") ||
            (equalc(tok, "thread_local") && opt_std_version && strcmp(opt_std_version, "202311L") >= 0)) {
            if (attr->is_tls)
                error_tok(tok, "duplicate 'thread_local'");
            if (attr->is_register)
                error_tok(tok, "'thread_local' used with 'register'");
            attr->is_tls = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "const") || equalc(tok, "__const") || equalc(tok, "__const__")) {
            quals |= QUAL_CONST;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "volatile") || equalc(tok, "__volatile") || equalc(tok, "__volatile__")) {
            quals |= QUAL_VOLATILE;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "restrict") || equalc(tok, "__restrict") || equalc(tok, "__restrict__")) {
            quals |= QUAL_RESTRICT;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Atomic")) {
            tok = tok->next;
            if (equalc(tok, "(") && is_typename(tok->next)) {
                tok = skip(tok, "(");
                ty = type_name(&tok, tok);
                tok = skip(tok, ")");
                ty = qualify_struct_type(ty, QUAL_ATOMIC);
            } else {
                quals |= QUAL_ATOMIC;
            }
            continue;
        }
        if (equalc(tok, "__cdecl") || equalc(tok, "__stdcall") || equalc(tok, "__fastcall") ||
            equalc(tok, "__thiscall") || equalc(tok, "__vectorcall")) {
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "signed") || equalc(tok, "__signed") || equalc(tok, "__signed__")) {
            is_signed = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "unsigned")) {
            is_unsigned = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "short")) {
            is_short = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "long")) {
            long_count++;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "int")) {
            is_int = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "char")) {
            is_char = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "float")) {
            is_float = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "double")) {
            is_double = true;
            tok = tok->next;
            continue;
        }
#ifndef __MUSL__
        if (equalc(tok, "_Decimal32")) {
            // IEEE 754-2008 decimal32 (7 digits, BID encoding). Was
            // previously aliased to float; now a real type whose ops go
            // through the __bid_* runtime.
            ty = ty_decimal32;
            parser_used_decimal = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Decimal64")) {
            ty = ty_decimal64;
            parser_used_decimal = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Decimal128")) {
            ty = ty_decimal128;
            parser_used_decimal = true;
            tok = tok->next;
            continue;
        }
#endif
        if (equalc(tok, "__bf16")) {
            // 16-bit storage (no real bf16 codegen); 2-byte elements so
            // avx512bf16vlintrin.h's `typedef __bf16 __v16bf` works.
            ty = ty_ushort;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Float16")) {
            is_float = true; // nearest supported type (matches the F16 suffix)
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Float32")) {
            is_float = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Float32x")) {
            is_double = true; // _Float32x >= double width -> double
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Float64")) {
            is_double = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Float64x")) {
            is_double = true; // _Float64x >= long double width -> long double
            long_count = 1;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Float128") || equalc(tok, "__float128")) {
            // ARM64 Linux: long double is true binary128; x86 long double is
            // 80-bit extended — _Float128/__float128 are not supported there.
            // Either way, alias to long double: real quad-precision codegen
            // isn't implemented, but library headers (e.g. fftw3.h's
            // __float128-gated quad-precision API declarations) still need
            // the type to parse and take a plausible (16-byte) size/align so
            // the surrounding typedefs/prototypes compile. Callers that
            // actually invoke quad-precision arithmetic are gated behind a
            // separate build-time feature macro (FFTW_QUAD, HAVE_FLOAT128,
            // etc.) and won't be compiled unless that feature is enabled.
#if defined(ARCH_ARM64) && !defined(__APPLE__)
            is_double = true;
            long_count = 1;
#else
            is_double = true;
            long_count = 1;
#endif
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Bool")) {
            is_bool = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_BitInt")) {
            tok = skip(tok->next, "(");
            Node *width_node = assign(&tok, tok);
            check_type(width_node);
            long long w;
            if (!eval_const_expr(width_node, &w))
                error_tok(tok, "_BitInt width must be a constant expression");
            if (w < 1)
                error_tok(tok, "_BitInt width must be positive");
            bitint_width = (int)w;
            is_bitint = true;
            tok = skip(tok, ")");
            continue;
        }
        if (equalc(tok, "_Complex") || equalc(tok, "__complex__")) {
            is_complex = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "__int64")) {
            long_count = 2;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "__int128")) {
            is_int128 = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "void")) {
            is_void = true;
            tok = tok->next;
            continue;
        }

        if (equalc(tok, "typeof")) {
            // typeof is only a keyword in C23+ or GNU mode; in C11 it's an identifier
            if (!opt_gnu_mode && (!opt_std_version || strcmp(opt_std_version, "202311L") < 0))
                break;
            tok = tok->next;
            tok = skip(tok, "(");
            if (is_typename(tok)) {
                ty = type_name(&tok, tok);
            } else {
                Node *node = expr(&tok, tok);
                check_type(node);
                ty = node->ty;
                queue_vm_typeof_eval(node, tok);
            }
            tok = skip(tok, ")");
            continue;
        }
        if (equalc(tok, "__typeof") || equalc(tok, "__typeof__")) {
            tok = tok->next;
            tok = skip(tok, "(");
            if (is_typename(tok)) {
                ty = type_name(&tok, tok);
            } else {
                Node *node = expr(&tok, tok);
                check_type(node);
                ty = node->ty;
                queue_vm_typeof_eval(node, tok);
            }
            tok = skip(tok, ")");
            continue;
        }

        // C23 typeof_unqual - same as typeof but strips all qualifiers recursively
        if (equalc(tok, "typeof_unqual") || equalc(tok, "__typeof_unqual") || equalc(tok, "__typeof_unqual__")) {
            // typeof_unqual is C23-only; __typeof_unqual/__typeof_unqual__ are GNU extensions
            if (equalc(tok, "typeof_unqual") && (!opt_std_version || strcmp(opt_std_version, "202311L") < 0))
                break;
            tok = tok->next;
            tok = skip(tok, "(");
            if (is_typename(tok))
                ty = type_name(&tok, tok);
            else {
                Node *node = expr(&tok, tok);
                check_type(node);
                ty = node->ty;
                queue_vm_typeof_eval(node, tok);
            }
            ty = type_unqual(ty);
            tok = skip(tok, ")");
            continue;
        }

        Typedef *td = find_typedef(tok);
        if (td) {
            // If we've already seen a built-in type specifier (int, char, etc.)
            // or another typedef/struct/enum type, stop: the typedef name is
            // likely the variable name, not a type specifier.
            if (is_int || is_char || is_short || long_count > 0 || is_float ||
                is_double || is_bool || is_void || is_signed || is_unsigned || is_int128 || ty)
                break;
            ty = td->ty;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "struct")) {
            ty = struct_or_union_specifier(&tok, tok, false);
            continue;
        }
        if (equalc(tok, "union")) {
            ty = struct_or_union_specifier(&tok, tok, true);
            continue;
        }
        if (equalc(tok, "enum")) {
            ty = enum_specifier(&tok, tok);
            continue;
        }
        break;
    }


    // C23: if 'auto' was seen without an explicit type specifier, treat as type inference.
    if (attr && has_auto_seen && !attr->is_auto_type) {
        bool explicit_type = is_int || is_char || is_short || long_count > 0 ||
            is_float || is_double || is_bool || is_void || is_signed || is_unsigned ||
            is_int128 || is_complex;
        if (!explicit_type && !ty)
            attr->is_auto_type = true;
    }
    if (!ty) {
        if (is_bitint) {
            ty = bitint_type(bitint_width, is_unsigned);
        } else if (is_void) {
            ty = ty_void;
        } else if (is_bool) {
            ty = ty_bool;
        } else if (is_float) {
            ty = ty_float;
        } else if (is_double && long_count >= 1) {
            ty = ty_ldouble;
        } else if (is_double) {
            ty = ty_double;
        } else if (is_char) {
            ty = is_unsigned ? ty_uchar : ty_char;
            if (is_char && is_signed && !is_unsigned) {
                ty = copy_type(ty_char);
                ty->is_unsigned = false;
                ty->is_signed_char = true;
            }
        } else if (is_short) {
            ty = is_unsigned ? ty_ushort : ty_short;
        } else if (is_int128) {
            ty = is_unsigned ? ty_uint128 : ty_int128;
        } else if (long_count >= 2) {
            ty = is_unsigned ? ty_ullong : ty_llong;
        } else if (long_count == 1) {
            ty = is_unsigned ? ty_ulong : ty_long;
        } else if (is_int || is_signed || is_unsigned) {
            ty = is_unsigned ? ty_uint : ty_int;
        } else if (is_complex) {
            ty = ty_double;
        } else {
            ty = ty_int;
            if (!attr->is_auto_type && !equalc(tok, "["))
                warn_tok(tok, "type defaults to int");
        }
    }


    if (is_complex && ty) {
        if (ty->size > 8 && !is_flonum(ty))
            error_tok(tok, "_Complex with %u-byte base type is not supported",
                      (unsigned)ty->size);
        ty = complex_type(ty);
    }

    if (!ty)
        error_tok(tok, "expected type name, got kind=%d text='%.20s'", tok->kind, tok->ptr);

    ty = apply_type_align(ty, attr_align);
    tok = skip_attributes(tok);
    quals |= collect_type_quals(&tok, tok);
    if (quals) {
        if (ty->kind == TY_ARRAY || ty->kind == TY_VLA) {
            ty = qualify_array_elem(ty, quals);
        } else {
            // qualify_struct_type(): never mutate a shared struct/union
            // type in place. A complete aggregate gets its own qualified
            // clone; an INCOMPLETE one gets a qualified variant that
            // struct_or_union_specifier() completes in lockstep with the
            // tag (see Type.qual_variants) -- mimalloc.h forward-declares
            // `struct mi_heap_s; typedef struct mi_heap_s mi_heap_t;`
            // and uses `const mi_heap_t*` before the type is ever
            // completed; stamping that const onto the shared mi_heap_s
            // type made the NON-const `mi_heap_t _mi_heap_main` in
            // init.c read as const too, and eval_const_expr()'s
            // ND_MEMBER fold (which correctly requires a const object)
            // folded `_mi_heap_main.thread_id == 0` to TRUE, making
            // `_mi_is_main_thread()` return constant 1 and every
            // thread share _mi_heap_main (multithreaded allocator
            // corruption). A qualifier on an incomplete type is
            // near-moot for member access anyway: no by-value use is
            // legal while it stays incomplete.
            ty = qualify_struct_type(ty, quals);
        }
    }
    // Apply a type-level vector_size attribute to the base type here, not in
    // declarator(), so every declarator of a multi-declarator declaration
    // inherits it: `vector(4,float) f1, f2;` must make f2 a vector too.
    // Trailing per-declarator attributes still apply in declarator().
    if (pending_vector_size) {
        // mode(SI) etc. adjusts the ELEMENT type and must be folded in before
        // the vector is built, or declarator()'s pending_mode handling would
        // shrink the whole vector to the element size.
        ty = apply_pending_mode(ty);
        ty = make_vector_type(ty, pending_vector_size);
        pending_vector_size = 0;
    }
    *rest = tok;
    return ty;
}

static Type *type_name(Token **rest, Token *tok) {
    VarAttr attr = {};
    bool _saved_in_type_name = in_type_name;
    // C23: prefix attributes cannot appear on type names
    if (equalc(tok, "[") && tok->next && equalc(tok->next, "[") &&
        tok->ptr + tok->len == tok->next->ptr)
        error_tok(tok, "expected type name, not an attribute specifier");
    in_type_name = true;
    Token *tn_start = tok;
    Type *base = declspec(&tok, tok, &attr);
    if (attr.is_typedef || attr.is_extern || attr.is_auto_type || attr.is_auto)
        error_tok(tn_start, "storage class specifier in type name");
    if (!in_compound_literal && (attr.is_static || attr.is_register || attr.is_tls))
        error_tok(tn_start, "storage class specifier in type name");
    Type *ty = declarator(&tok, tok, copy_type(base), NULL, &attr);
    in_type_name = _saved_in_type_name;
    tok = skip_attributes(tok);
    *rest = tok;
    return ty;
}

static Type *parse_cast_type(Token **rest, Token *tok) {
    // Compound literals (type){init} allow _Alignas in the type name;
    // peek past the matching ')' for '{'.
    bool saved_icl = in_compound_literal;
    int depth = 0;
    for (Token *t = tok; t && t->kind != TK_EOF; t = t->next) {
        if (equalc(t, "("))
            depth++;
        else if (equalc(t, ")")) {
            if (--depth == 0) {
                if (equalc(t->next, "{"))
                    in_compound_literal = true;
                break;
            }
        }
    }
    tok = skip(tok, "(");
    Type *ty = type_name(&tok, tok);
    *rest = skip(tok, ")");
    in_compound_literal = saved_icl;
    return ty;
}

static bool is_cast(Token *tok) {
    if (!equalc(tok, "("))
        return false;
    tok = tok->next;
    tok = skip_attributes(tok);
    return is_typename(tok);
}

static Node *stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);

static int array_len(Type *ty) {
    if (!ty || ty->kind != TY_ARRAY || !ty->base || ty->base->size == 0)
        return 0;
    return ty->size / ty->base->size;
}

// Insert `rel` into var->relocs, sorted by offset (dedup: a later reloc at
// the same offset replaces an earlier one — e.g. a designated-initializer
// override). Shared by append_reloc() and append_label_diff_reloc() below.
static void insert_reloc_sorted(LVar *var, Reloc *rel) {
    int offset = rel->offset;
    if (!var->relocs || var->relocs->offset > offset) {
        rel->next = var->relocs;
        var->relocs = rel;
        return;
    }

    // Replace head if offset matches
    if (var->relocs->offset == offset) {
        rel->next = var->relocs->next;
        var->relocs = rel;
        return;
    }

    Reloc *cur = var->relocs;
    while (cur->next && cur->next->offset < offset)
        cur = cur->next;

    if (cur->next && cur->next->offset == offset) {
        // Replace existing reloc at same offset (designator override)
        rel->next = cur->next->next;
        cur->next = rel;
    } else {
        rel->next = cur->next;
        cur->next = rel;
    }
}

static void append_reloc(LVar *var, int offset, char *label, int addend) {
    Reloc *rel = arena_alloc(sizeof(Reloc));
    rel->offset = offset;
    rel->label = label;
    rel->addend = addend;
    insert_reloc_sorted(var, rel);
    // Mark the target function (if any) address-taken: codegen.c's
    // plain-inline-with-no-forcing-declaration SB_LOCAL/SB_WEAK choice
    // (see rcc.h's LVar.addr_taken doc comment) consults this so `&fn`
    // in a global initializer gets a linker-collapsible symbol, while a
    // same-shaped function only ever *called* keeps the narrower,
    // no-cross-TU-visibility-needed SB_LOCAL binding.
    LVar *target = find_global_name(label);
    if (target && target->is_function)
        target->addr_taken = true;
}

// Label-address DIFFERENCE (GCC's `&&label_a - &&label_b` computed-goto
// jump-table idiom, e.g. torture/pr70460.c's `static int b[] = { &&lab1 -
// &&lab0, &&lab2 - &&lab0 };`). Unlike append_reloc()'s single-symbol case
// (a real ELF/Mach-O relocation, resolved at link/object-write time), this
// is resolved by codegen.c as a same-object byte patch once both labels'
// .text offsets are known — there is no relocation kind that expresses
// "symbol A minus symbol B". `size` is the patch width in bytes (the
// initializer element's own type size: 1/2/4/8).
static void append_label_diff_reloc(LVar *var, int offset, char *label_hi, char *label_lo, int size) {
    Reloc *rel = arena_alloc(sizeof(Reloc));
    rel->offset = offset;
    rel->label = label_hi;
    rel->label2 = label_lo;
    rel->size = size;
    insert_reloc_sorted(var, rel);
}

// Recognize "&&label_a - &&label_b" (both address-of-label expressions,
// after unwrapping casts on the whole expression and each operand) for
// label-difference initializers. Returns the resolved ".L.label.<fn>.<name>"
// symbol names for both operands, matching cg_def_label()'s naming in
// codegen.c (ND_LABEL case) and read_global_label_initializer()'s
// single-label case below. A bare label (no enclosing function) can't
// occur here: labels only exist inside function bodies, so parser_current_fn
// is always set while parsing a static initializer that references one.
static bool extract_label_diff(Node *node, char **label_hi, char **label_lo) {
    while (node && node->kind == ND_CAST) node = node->lhs;
    if (!node || node->kind != ND_SUB) return false;
    Node *lhs = node->lhs, *rhs = node->rhs;
    while (lhs && lhs->kind == ND_CAST) lhs = lhs->lhs;
    while (rhs && rhs->kind == ND_CAST) rhs = rhs->lhs;
    if (!lhs || !rhs || lhs->kind != ND_LABEL_VAL || rhs->kind != ND_LABEL_VAL)
        return false;
    const char *fn_hi = lhs->funcname ? lhs->funcname : parser_current_fn;
    const char *fn_lo = rhs->funcname ? rhs->funcname : parser_current_fn;
    *label_hi = fn_hi ? format(".L.label.%s.%s", fn_hi, lhs->label_name) : lhs->label_name;
    *label_lo = fn_lo ? format(".L.label.%s.%s", fn_lo, rhs->label_name) : rhs->label_name;
    return true;
}

static bool read_global_label_initializer(Token **rest, Token *tok, char **label, int *addend) {
    if (tok->kind == TK_STR) {
        StrLit *s = new_str_lit(tok->str, tok->len, tok->string_literal_prefix,
                                str_lit_elem_size(tok->string_literal_prefix));
        *label = format(".LC%d", s->id);
        if (addend) *addend = 0;
        *rest = tok->next;
        // Handle "string" + const or "string" - const
        if (equalc(*rest, "+") || equalc(*rest, "-")) {
            bool is_sub = equalc(*rest, "-");
            Token *op_next = (*rest)->next;
            Node *n = assign(&op_next, op_next);
            long long v;
            if (eval_const_expr(n, &v)) {
                if (addend) *addend += is_sub ? -(int)v : (int)v;
                *rest = op_next;
            }
        }
        return true;
    }

    while (is_cast(tok))
        parse_cast_type(&tok, tok);

    // GCC label address: &&label -- checked AFTER stripping any leading
    // casts (e.g. `(void*)&&label`, the shape a computed-goto dispatch
    // table's initializer list always uses, since the table's element
    // type is typically `void *` or a typedef'd function-pointer-ish
    // handler type rather than the label's own implicit type). Checking
    // this before cast-stripping (the previous position) only matched a
    // bare, uncast `&&label`; found via a real PHP build:
    // Zend/zend_vm_execute.h's HYBRID VM dispatch table
    //   static zend_vm_opcode_handler_t const labels[] = {
    //       (void*)&&ZEND_NOP_SPEC_LABEL, ...
    //   };
    if (equalc(tok, "&&") && tok->next && tok->next->kind == TK_IDENT) {
        if (parser_current_fn)
            *label = format(".L.label.%s.%s", parser_current_fn, tok->next->name);
        else
            *label = tok->next->name;
        if (addend) *addend = 0;
        *rest = tok->next->next;
        return true;
    }

    if (equalc(tok, "&"))
        tok = tok->next;

    if (tok->kind == TK_IDENT) {
        // Not a symbol reference: enum constants and the true/false/NULL/
        // nullptr keywords fold to integer constants via the expression path.
        // _Generic is tokenized as a plain identifier too (no dedicated
        // token kind), but "_Generic(...)" starts a full selection
        // expression, not a bare symbol name — without this check it fell
        // into the plain-identifier branch below, which treated the
        // literal text "_Generic" itself as the label to reference and
        // stopped right there (leaving the caller staring at the
        // following "(" and reporting a confusing "expected ';' or ','"
        // several tokens later). Found via a real Linux kernel build:
        // init/version-timestamp.c's init_uts_ns initializer selects its
        // .ops field's proc_ns_operations pointer via
        // _Generic((&init_uts_ns), struct foo *: &foo_operations, ...).
        // Any C keyword/builtin token can never name a global object — in
        // particular `sizeof` (and the true/false/NULL/nullptr/_Generic/
        // __builtin_* cases listed below), which were previously taken as
        // a label reference, so `var x = sizeof(struct T);` emitted a
        // reloc to a nonexistent symbol named "sizeof" and then choked on
        // the trailing "(...)" (cello's CelloObject initializes pointer
        // fields with `(var)sizeof(struct T)`).
        if (tok->kw != ID_NONE || find_enum_const(tok) || equalc(tok, "true") ||
            equalc(tok, "false") || equalc(tok, "NULL") || equalc(tok, "nullptr") ||
            equalc(tok, "_Generic") ||
            (tok->len > 10 && !memcmp(tok->ptr, "__builtin_", 10)))
            return false;
        // Use asm_name for static local variables (mangled labels)
        LVar *lv = find_global_name(tok->name);
        if (!lv)
            for (LVar *v = locals; v; v = v->next)
                if (v->name == tok->name && !v->is_local) {
                    lv = v;
                    break;
                }
        *label = (lv && lv->asm_name) ? lv->asm_name : tok->name;
        if (addend) *addend = 0;
        *rest = tok->next;

        // Handle chained &identifier[N][M].member[N]... access
        LVar *lv2 = find_global_name(*label);
        Type *cur_ty = lv2 ? lv2->ty : NULL;
        while (cur_ty) {
            if (equalc(*rest, "[") && (cur_ty->kind == TY_ARRAY || cur_ty->kind == TY_PTR)) {
                Token *sub = (*rest)->next;
                Node *idx = assign(&sub, sub);
                check_type(idx);
                long long ival;
                if (sub->kind != TK_EOF && equalc(sub, "]") && eval_const_expr(idx, &ival)) {
                    int elem_size = cur_ty->base ? cur_ty->base->size : 1;
                    if (addend) *addend += (int)(ival * elem_size);
                    cur_ty = cur_ty->base;
                    *rest = sub->next;
                } else
                    break;
            } else if (equalc(*rest, ".") && cur_ty && (cur_ty->kind == TY_STRUCT || cur_ty->kind == TY_UNION)) {
                Token *member_tok = (*rest)->next;
                if (member_tok && member_tok->kind == TK_IDENT) {
                    Member *mem = find_member(cur_ty, member_tok);
                    if (mem) {
                        if (addend) *addend += mem->offset;
                        cur_ty = mem->ty;
                        *rest = member_tok->next;
                    } else
                        break;
                } else
                    break;
            } else
                break;
        }

        // Handle "identifier[...].member... + const" / "- const" — the
        // same address-plus-offset idiom already supported for a string
        // literal a few lines up, but for a plain symbol reference (with
        // or without a preceding [index]/.member chain). Real kernel
        // case: arch/x86/kernel/alternative.c's
        // `x86nops + 1 + 2 + ...` computing sub-array start addresses.
        if (equalc(*rest, "+") || equalc(*rest, "-")) {
            bool is_sub = equalc(*rest, "-");
            Token *op_next = (*rest)->next;
            Node *n = assign(&op_next, op_next);
            long long v;
            if (eval_const_expr(n, &v)) {
                // Pointer arithmetic scaling: "table + N" advances by
                // N elements of the pointee type, not N bytes (cur_ty
                // is the array/pointer type at this point in the
                // chain — its ->base is the pointee). Real bug: a
                // `struct config_enum_entry table[]; = table + 1`
                // reloc addend must be N*sizeof(struct entry), not N;
                // without scaling every non-byte-sized array/pointer
                // "+const" initializer (e.g. PostgreSQL's
                // ssl_protocol_versions_info + 1) landed mid-element.
                int elem_size = (cur_ty && cur_ty->base) ? cur_ty->base->size : 1;
                if (addend) *addend += is_sub ? -(int)v * elem_size : (int)v * elem_size;
                *rest = op_next;
            }
        }
        return true;
    }

    return false;
}

// Extract a symbol + addend from an expression tree for global initializer relocs.
// Handles: &var, &var.member, &var[const], &(var+const)->member, &("string"[n]), etc.
static bool extract_reloc(Node *node, char **label, int *addend) {
    if (!node) return false;
    char *lbl = NULL, *rbl = NULL;
    int ladd = 0, radd = 0;
    switch (node->kind) {
    case ND_LVAR:
        if (node->var && !node->var->is_local) {
            *label = node->var->asm_name ? node->var->asm_name : node->var->name;
            *addend = 0;
            return true;
        }
        return false;
    case ND_STR:
        // Reuse the StrLit already registered by primary() (node->str_id)
        // instead of re-registering with the wrong, hardcoded prefix=0/
        // elem_size=1 -- a wide/char16_t/char32_t string reached here
        // (e.g. "&L\"text\"", or a wide literal as one array-of-pointers
        // initializer element) would otherwise get a SECOND, WRONGLY-
        // TAGGED StrLit: codegen.c's emission loop keys off elem_size to
        // both align the literal (glibc's wcslen/wmemcmp assume
        // _Alignof(wchar_t)) and choose the 2-or-4-byte-per-character
        // encoding, so elem_size=1 for what's actually 4-byte-per-char
        // data produced a misaligned, narrow-packed duplicate distinct
        // from the correct one the plain (non-reloc) read path uses.
        *label = format(".LC%d", node->str_id);
        *addend = 0;
        return true;
    case ND_NUM:
        if (node->val != (long long)(int)node->val)
            return false; // int addend overflow: fall back to the full-width scalar eval
        *label = NULL;
        *addend = (int)node->val;
        return true;
    case ND_ADD:
        if (extract_reloc(node->lhs, &lbl, &ladd) && extract_reloc(node->rhs, &rbl, &radd)) {
            if (!rbl) {
                *label = lbl;
                *addend = ladd + radd;
                return true;
            }
            if (!lbl) {
                *label = rbl;
                *addend = ladd + radd;
                return true;
            }
        }
        return false;
    case ND_MUL: {
        long long lv, rv;
        if (eval_const_expr(node->lhs, &lv) && eval_const_expr(node->rhs, &rv)) {
            *label = NULL;
            *addend = (int)(lv * rv);
            return true;
        }
        return false;
    }
    case ND_SUB: {
        // "label - const" (an address constant minus a byte offset) —
        // the same address-arithmetic idiom ND_ADD already supports on
        // either side, but subtraction isn't commutative: only the
        // *left* side may hold the label. Tried first so a genuine
        // address expression resolves via a real relocation instead of
        // falling through to eval_const_expr(), which can't fold an
        // address at all and always fails for it — the fallback below
        // (unchanged from before) still covers every purely-constant
        // subtraction this case previously handled. Real kernel case:
        // arch/x86/kernel/cpu/common.c's cpu_current_top_of_stack percpu
        // initializer: `(unsigned long)&init_stack + sizeof(init_stack)
        // - TOP_OF_KERNEL_STACK_PADDING`.
        if (extract_reloc(node->lhs, &lbl, &ladd)) {
            long long rv;
            if (eval_const_expr(node->rhs, &rv)) {
                *label = lbl;
                *addend = ladd - (int)rv;
                return true;
            }
        }
        long long v;
        if (eval_const_expr(node, &v)) {
            *label = NULL;
            *addend = (int)v;
            return true;
        }
        return false;
    }
    case ND_SHL:
    case ND_SHR:
    case ND_BITAND:
    case ND_BITXOR:
    case ND_BITOR:
    case ND_DIV:
    case ND_MOD:
    case ND_NEG:
    case ND_NOT:
    case ND_BITNOT: {
        long long v;
        if (eval_const_expr(node, &v)) {
            if (v != (long long)(int)v)
                return false; // int addend overflow: fall back to the full-width scalar eval
            *label = NULL;
            *addend = (int)v;
            return true;
        }
        return false;
    }
    case ND_CAST:
        return extract_reloc(node->lhs, label, addend);
    case ND_ADDR:
        // &*x = x : ND_ADDR(ND_DEREF(x)) -> skip the ADDR/DEREF pair
        if (node->lhs->kind == ND_DEREF)
            return extract_reloc(node->lhs->lhs, label, addend);
        // offsetof pattern &((struct S*)0)->member
        return extract_reloc(node->lhs, label, addend);
    case ND_DEREF:
        return extract_reloc(node->lhs, label, addend);
    case ND_MEMBER:
        if (extract_reloc(node->lhs, &lbl, &ladd)) {
            *label = lbl;
            *addend = ladd + node->member->offset;
            return true;
        }
        return false;
    case ND_COND: {
        long long cv;
        if (!eval_const_expr(node->cond, &cv))
            return false;
        return extract_reloc(cv ? node->then : node->els, label, addend);
    }
    case ND_COMMA:
        return extract_reloc(node->rhs, label, addend);
    default:
        return false;
    }
}

// Does `node` (after stripping casts) look like a genuine address
// computation (&sym, &sym +/- N, a string literal, ...) rather than a
// plain value read? Guards the integer-scalar extract_reloc() fallback
// below it: extract_reloc()'s ND_LVAR case treats *any* reference to a
// global as "the address of that global" once it's reached as a leaf —
// correct only when the expression as a whole is already known to
// represent an address, which every existing (pointer-typed) call site
// guarantees by construction. A bare non-constant read like
// "static int vi = some_other_global;" must remain a hard "not a
// constant expression" error, not silently become "the address of
// some_other_global" (c11-thread-local-2.c's "static _Thread_local int
// vi = vv;" — a torture dg-error case — caught this exact overreach).
static bool looks_like_address_expr(Node *node) {
    while (node && node->kind == ND_CAST) node = node->lhs;
    if (!node) return false;
    switch (node->kind) {
    case ND_ADDR:
    case ND_STR:
        return true;
    case ND_LVAR:
        // A bare function name, or a bare *array*-typed variable name (no
        // explicit '&' on either), always implicitly decays to its
        // address — there's no other valid meaning for either in a
        // scalar initializer, per C's function/array-to-pointer decay
        // rules — unlike a bare *scalar* data variable reference, which
        // extract_reloc()'s ND_LVAR case would otherwise also treat as
        // "the address of", too permissively (see the c11-thread-local-2
        // regression this function was written to guard against). Real
        // kernel case: arch/x86/kernel/idt.c's
        // `.address = (unsigned long) idt_table` (idt_table is a plain
        // array, no '&') — array decay, not a value read.
        return node->var && (node->var->is_function || (node->var->ty && node->var->ty->kind == TY_ARRAY));
    case ND_ADD:
    case ND_SUB:
        return looks_like_address_expr(node->lhs) || looks_like_address_expr(node->rhs);
    case ND_COND: {
        // A compile-time-constant condition selecting between "0" and a
        // real address — e.g. linux/pci.h-style driver tables' common
        // "IS_ENABLED(CONFIG_X) ? 0 : (kernel_ulong_t)&"literal"" idiom
        // for optionally embedding a diagnostic string address in an
        // otherwise-plain integer field. Only the *selected* branch needs
        // to look like an address — the other is never evaluated, exactly
        // like extract_reloc()'s own ND_COND case just below, which this
        // gate must agree with or a selected address branch never even
        // reaches it.
        long long cv;
        if (!eval_const_expr(node->cond, &cv))
            return false;
        return looks_like_address_expr(cv ? node->then : node->els);
    }
    default:
        return false;
    }
}

static Token *skip_initializer(Token *tok) {
    // Skip designated initializer: .name = value
    if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
        tok = tok->next->next;
        if (equalc(tok, "="))
            tok = tok->next;
        return skip_initializer(tok);
    }
    // Skip array index designator: [N] = value or [N ... M] = value
    if (equalc(tok, "[")) {
        int depth = 1;
        tok = tok->next;
        while (depth > 0 && tok->kind != TK_EOF) {
            if (equalc(tok, "[")) depth++;
            else if (equalc(tok, "]"))
                depth--;
            tok = tok->next;
        }
        if (equalc(tok, "...")) {
            tok = tok->next;
            while (!equalc(tok, "]") && tok->kind != TK_EOF) tok = tok->next;
            tok = tok->next;
        }
        if (equalc(tok, "=")) tok = tok->next;
        return skip_initializer(tok);
    }
    if (!equalc(tok, "{")) {
        assign(&tok, tok);
        return tok;
    }

    int depth = 0;
    do {
        if (equalc(tok, "{"))
            depth++;
        else if (equalc(tok, "}"))
            depth--;
        tok = tok->next;
    } while (depth > 0 && tok->kind != TK_EOF);
    return tok;
}

static Token *skip_flat_aggregate_init(Token *tok, Type *ty) {
    if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
        Member *mem = ty->members;
        while (mem) {
            if (equalc(tok, "}"))
                break;
            tok = skip_flat_aggregate_init(tok, mem->ty);
            // A union flat-initializes exactly ONE member (C11 6.7.9p13;
            // extra values are excess elements, warned by gcc). Its
            // trailing comma separates the NEXT ARRAY ELEMENT, not another
            // member — consuming it here made count_array_initializer()
            // miscount `union U a[] = { 1, 2 }` as one element and then
            // choke on the leftover `2` ("expected specific operator",
            // janet's Janet[] = { expr, tstate.payload }).
            if (ty->kind == TY_UNION)
                break;
            mem = mem->next;
            if (mem && equalc(tok, ","))
                tok = tok->next;
        }
    } else if (ty->kind == TY_ARRAY) {
        if (equalc(tok, "{") || (ty->base->kind == TY_CHAR && tok->kind == TK_STR)) {
            tok = skip_initializer(tok);
        } else {
            int len = array_len(ty);
            for (int i = 0; i < len && !equalc(tok, "}"); i++) {
                tok = skip_flat_aggregate_init(tok, ty->base);
                if (i < len - 1 && equalc(tok, ","))
                    tok = tok->next;
            }
        }
    } else {
        assign(&tok, tok);
    }
    return tok;
}

// Evaluate a constant integer expression without consuming the tokens permanently.
static long long peek_const_expr(Token *tok) {
    Token *tmp = tok;
    Node *node = assign(&tmp, tmp);
    check_type(node);
    long long val = 0;
    if (!eval_const_expr(node, &val))
        return -1;
    return val;
}

static Token *find_compound_literal_start(Token *tok);

static int count_array_initializer(Token **rest, Token *tok, Type *elem_ty) {
    int count = 0;
    int max_idx = -1;
    int idx = 0;
    tok = skip(tok, "{");
    while (!equalc(tok, "}")) {
        int eidx = idx;
        bool member_designator = false;
        if (equalc(tok, "[")) {
            tok = tok->next; // skip [
            long long aidx = peek_const_expr(tok);
            assign(&tok, tok); // skip first expression
            eidx = (int)aidx;
            if (equalc(tok, "...")) {
                tok = tok->next; // skip ...
                long long aeidx = peek_const_expr(tok);
                assign(&tok, tok); // skip second expression
                eidx = (int)aeidx;
            }
            if (eidx > max_idx) max_idx = eidx;
            tok = skip(tok, "]");
            // Combined designator [N].member[.sub]*=val (C99 6.7.8p17):
            // skip any ".member" chain before the "=". Its value belongs
            // to just that member, not the whole (possibly struct/union)
            // element, so it must NOT fall into the elem_ty-based
            // struct/union heuristic below — that would wrongly try to
            // flat-initialize the entire element from what's really a
            // single member's value.
            while (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                tok = tok->next->next;
                member_designator = true;
            }
            tok = skip(tok, "=");
        }
        if (!member_designator && elem_ty && (elem_ty->kind == TY_STRUCT || elem_ty->kind == TY_UNION) && !equalc(tok, "{")) {
            // Heuristic: if the first token is an identifier of struct/union type,
            // or a compound literal, treat it as a single element expression.
            // Otherwise use flat aggregate initialization.
            bool is_struct_expr = false;
            if (tok->kind == TK_IDENT) {
                LVar *var = find_var(tok);
                if (var && var->ty && (var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION))
                    is_struct_expr = true;
                // A struct/union-returning function call is a single element
                // expression (e.g. `struct S a[] = { mk0(), mk1() }`), not a
                // flat aggregate of the struct's members. A function's global
                // LVar is typed pointer-to-function (TY_PTR -> TY_FUNC), so
                // check the pointee's return type. Without this,
                // count_array_initializer() routed `mk0()` through
                // skip_flat_aggregate_init(), which consumed `mk0()` as the
                // first member and `mk1()` as the second -- miscounting the
                // array as 1 element and dropping the second initializer call
                // (jerryscript's test-ext-arg: the second jerryx_arg_t in the
                // mapping[] array ended up NULL).
                else if (var && var->ty && var->ty->kind == TY_PTR && var->ty->base &&
                         var->ty->base->kind == TY_FUNC && var->ty->base->return_ty &&
                         (var->ty->base->return_ty->kind == TY_STRUCT ||
                          var->ty->base->return_ty->kind == TY_UNION) &&
                         tok->next && equalc(tok->next, "("))
                    is_struct_expr = true;
            } else if (find_compound_literal_start(tok)) {
                is_struct_expr = true;
            }
            if (is_struct_expr) {
                tok = skip_initializer(tok);
            } else {
                tok = skip_flat_aggregate_init(tok, elem_ty);
            }
        } else {
            tok = skip_initializer(tok);
        }
        if (eidx > max_idx) max_idx = eidx;
        count++;
        idx = eidx + 1;
        if (equalc(tok, ",")) {
            tok = tok->next;
            if (equalc(tok, "}"))
                break;
            continue;
        }
        break;
    }
    *rest = skip(tok, "}");
    return max_idx >= count ? max_idx + 1 : count;
}

static Type *infer_array_type(Type *ty, Token *tok) {
    if (!ty || ty->kind != TY_ARRAY || ty->size != 0)
        return ty;
    // Only a character/wide-character ARRAY ELEMENT is ever sized by a
    // string literal's own length (C11 6.7.9p14). An array of TY_PTR
    // (e.g. `const char *arr[] = { "vec_" }`, a ONE-element array of
    // pointers, each pointer-initialized FROM a string literal) has
    // nothing to do with 6.7.9p14 and must fall through to
    // count_array_initializer()'s per-element count instead. Mirrors
    // global_init_one()'s own scalarish_base guard further below in
    // this file, which excludes the same TY_ARRAY/TY_STRUCT/TY_UNION/
    // TY_PTR kinds for the identical reason (TY_CHAR alone would be too
    // narrow -- it would also wrongly exclude a wide-char array, whose
    // base kind is an ordinary integer type like TY_SHORT/TY_INT, not
    // TY_CHAR). Missing this guard let `const char *arr[] = { "vec_" }`
    // get sized as strlen("vec_")+1 == 5 elements instead of the
    // correct 1 (found via flatcc's fb_reserved_kw_vec_prefixes[] = {
    // "vec_" }: the resulting 4 phantom trailing elements walked into
    // adjacent .rodata/unmapped memory and crashed on garbage passed
    // to strlen()).
    bool scalarish_base = ty->base->kind != TY_ARRAY && ty->base->kind != TY_STRUCT &&
        ty->base->kind != TY_UNION && ty->base->kind != TY_PTR;
    // "{ STRLIT }" / "{ STRLIT, }" is a superfluous-but-legal single-element
    // brace around a string literal (C11 6.7.9p14) — exactly equivalent to
    // the bare STRLIT form handled just below. Unwrap it first so the size
    // is computed from the string's own length, not from
    // count_array_initializer()'s generic per-element count (which would
    // see one initializer-list element and infer length 1 — global_init_one()
    // below already unwraps this same shape before writing the actual
    // bytes; this function must agree on the array's size).
    if (scalarish_base && equalc(tok, "{") && tok->next && tok->next->kind == TK_STR) {
        Token *after = tok->next->next;
        if (equalc(after, ",")) after = after->next;
        if (equalc(after, "}"))
            tok = tok->next; // unwrap: point straight at the string literal
    }
    // A parenthesized string chain — `char s[] = ( "a" "b" )` — is still
    // a string-literal initializer (C11 6.7.9p14), e.g. diffutils'
    // C_ifdef_group_formats. Peek past the parens to size the array from
    // the (first) string's length; do NOT advance `tok`, so the initializer
    // itself still parses the parens as an ordinary expression.
    if (scalarish_base && equalc(tok, "(") && tok->next && tok->next->kind == TK_STR) {
        Token *inner = tok->next;
        if (inner->string_literal_prefix == 0 || inner->string_literal_prefix == '8')
            return array_of(ty->base, inner->len + 1);
        int n = 0;
        for (char *p = inner->str, *end = p + inner->len; p < end; n++) {
            char *next_p;
            decode_utf8(&next_p, p);
            p = next_p;
        }
        return array_of(ty->base, n + 1);
    }
    if (scalarish_base && tok->kind == TK_STR) {
        if (tok->string_literal_prefix == 0 || tok->string_literal_prefix == '8')
            return array_of(ty->base, tok->len + 1);
        // For wide strings, count UTF-8 codepoints bounded by tok->len, NOT
        // NUL-terminated utf8_len(): a wide literal's decoded byte buffer
        // may legitimately contain an embedded NUL codepoint before its
        // real end (e.g. lz4/libarchive-style "\0KMGTPEZY" lookup tables),
        // and utf8_len()'s `while (*p)` loop stopped at the FIRST one,
        // sizing the array as if the literal were empty (1 element instead
        // of the true length).
        int n = 0;
        for (char *p = tok->str, *end = p + tok->len; p < end; n++) {
            char *next_p;
            decode_utf8(&next_p, p);
            p = next_p;
        }
        return array_of(ty->base, n + 1);
    }
    if (equalc(tok, "{")) {
        Token *tmp = tok;
        int len = count_array_initializer(&tmp, tmp, ty->base);
        return array_of(ty->base, len);
    }
    return ty;
}

// Detect compound literal like (type){...} or ((type){...}) and return
// a pointer to the { token, or NULL if not a compound literal.
static Token *find_compound_literal_start(Token *tok) {
    Token *t = tok;
    for (;;) {
        while (equalc(t, "("))
            t = t->next;
        if (!is_typename(t))
            return NULL;
        // Lookahead for a compound-literal type name: _Alignas is allowed here
        // (C23 6.5.2.5), so suppress the type-name alignment diagnostic.
        bool saved_icl = in_compound_literal;
        in_compound_literal = true;
        Type *ty = type_name(&t, t);
        in_compound_literal = saved_icl;
        if (!ty)
            return NULL;
        while (equalc(t, ")"))
            t = t->next;
        if (equalc(t, "{"))
            return t;
        // A cast wrapping a compound literal -- "(T)(T2{...})", e.g. Cello's
        // `(var)((var[]){ ... })` -- has another parenthesized type between
        // the cast's ")" and the literal's "{". Parse that inner type too.
        if (!equalc(t, "("))
            return NULL;
    }
}

// After "&(compound literal)" is folded into a reference to a materialized
// static object, a trailing chain of constant array subscripts and/or
// member accesses may still follow before the enclosing initializer
// context resumes — e.g. drivers/gpu/drm/i915/i915_pmu.c's
// I915_PMU_FORMAT_ATTR(): "&((struct i915_str_attribute[]){...})[0].attr.attr".
// Fold the whole chain into one constant byte addend on the relocation
// here, rather than leaving it for the caller to (mis)parse as a separate
// initializer element. `ty` is the compound literal's own type (array or
// struct/union); a bare "&(compound literal)" with nothing trailing simply
// falls straight through the loop with addend 0, unchanged from before.
static int parse_const_addend_chain(Token **rest, Token *tok, Type *ty) {
    int addend = 0;
    for (;;) {
        if (equalc(tok, "[") && ty && ty->kind == TY_ARRAY) {
            tok = tok->next;
            Node *n = assign(&tok, tok);
            long long idx = 0;
            eval_const_expr(n, &idx);
            tok = skip(tok, "]");
            addend += (int)idx * (int)(ty->base ? ty->base->size : 0);
            ty = ty->base;
            continue;
        }
        if (equalc(tok, ".") && ty && (ty->kind == TY_STRUCT || ty->kind == TY_UNION)) {
            tok = tok->next;
            Member *mem = find_member(ty, tok);
            if (!mem)
                error_tok(tok, "no such member");
            addend += mem->offset;
            ty = mem->ty;
            tok = tok->next;
            continue;
        }
        break;
    }
    *rest = tok;
    return addend;
}

static void remove_reloc(LVar *var, int offset) {
    if (!var->relocs) return;
    if (var->relocs->offset == offset) {
        var->relocs = var->relocs->next;
        return;
    }
    for (Reloc *cur = var->relocs; cur->next; cur = cur->next) {
        if (cur->next->offset == offset) {
            cur->next = cur->next->next;
            return;
        }
    }
}

static void ensure_init_size(LVar *var, int offset, int size) {
    int need = offset + size;
    if (need > var->init_size) {
        char *new_data = arena_alloc(need);
        if (var->init_data) {
            memcpy(new_data, var->init_data, var->init_size);
            memset(new_data + var->init_size, 0, need - var->init_size);
        }
        var->init_data = new_data;
        var->init_size = need;
    }
}

static void write_scalar_bytes(LVar *var, int offset, int size, int64_t val) {
    if (offset < 0) return;
    ensure_init_size(var, offset, size);
    // Remove any reloc at this offset (scalar value overrides pointer reloc)
    remove_reloc(var, offset);
    if (size == 1) {
        var->init_data[offset] = (char)val;
        return;
    }
    if (size == 2) {
        int16_t v = (int16_t)val;
        memcpy(var->init_data + offset, &v, 2);
        return;
    }
    if (size == 4) {
        int32_t v = (int32_t)val;
        memcpy(var->init_data + offset, &v, 4);
        return;
    }
    int64_t v = val;
    memcpy(var->init_data + offset, &v, 8);
}

// Forward declaration
static Token *global_init_one(Token *tok, LVar *var, Type *ty, int offset);

static Token *global_init_flat_array(Token *tok, LVar *var, Type *ty, int offset) {
    if (ty->kind == TY_ARRAY) {
        int len = array_len(ty);
        Type *base = ty->base;
        int elem_size = base->size;
        for (int i = 0; i < len && !equalc(tok, "}"); i++) {
            tok = global_init_flat_array(tok, var, base, offset + i * elem_size);
            if (i < len - 1 && equalc(tok, ","))
                tok = tok->next;
        }
        return tok;
    }
    return global_init_one(tok, var, ty, offset);
}

static Token *global_init_member(Token *tok, LVar *var, Member *mem, int base_offset) {
    if (mem->bit_width > 0) {
        Node *node = assign(&tok, tok);
        check_type(node);
        long long val = 0;
        if (eval_const_expr(node, &val)) {
            int off = base_offset + mem->offset;
            int unit_sz = mem->ty->size;
            unsigned long long mask;
            unsigned long long new_val;
            if (mem->bit_width == 64) {
                mask = ~0ULL << mem->bit_offset;
                new_val = val << mem->bit_offset;
            } else {
                mask = ((1ULL << mem->bit_width) - 1) << mem->bit_offset;
                new_val = ((val & ((1ULL << mem->bit_width) - 1)) << mem->bit_offset);
            }
            if (unit_sz == 1) {
                unsigned char old = var->init_data[off];
                var->init_data[off] = (old & ~mask) | new_val;
            } else if (unit_sz == 2) {
                uint16_t old;
                memcpy(&old, var->init_data + off, 2);
                old = (old & ~mask) | new_val;
                memcpy(var->init_data + off, &old, 2);
            } else if (unit_sz == 4) {
                uint32_t old;
                memcpy(&old, var->init_data + off, 4);
                old = (old & ~mask) | new_val;
                memcpy(var->init_data + off, &old, 4);
            } else {
                uint64_t old;
                memcpy(&old, var->init_data + off, 8);
                old = (old & ~mask) | new_val;
                memcpy(var->init_data + off, &old, 8);
            }
        }
        return tok;
    }
    // A parenthesized string literal ("...") — e.g. glib's N_("%.1f kB")
    // — is still a string literal (C11 6.7.9p14) for a char-array member;
    // let global_init_one's paren-unwrap handle it instead of the
    // element-wise flat-array path (which wrote the literal index byte).
    if (mem->ty->kind == TY_ARRAY && !equalc(tok, "{") && tok->kind != TK_STR &&
        !(equalc(tok, "(") && tok->next && tok->next->kind == TK_STR)) {
        return global_init_flat_array(tok, var, mem->ty, base_offset + mem->offset);
    }
    // GCC: an extra brace level around a scalar MEMBER's own initializer
    // (e.g. `struct { int *p; } v = { { 0 } }`) is redundant -- unlike a
    // bare scalar's own optionally-braced initializer (C11 6.7.9p11,
    // exempt), which global_init_one() also handles via this same
    // recursive call but never as a struct member.
    // Suppress during a speculative constexpr re-parse (a compound
    // literal's runtime-codegen pass already warned once on the real
    // parse; the fold-attempt re-parse of the same tokens must stay
    // silent, matching every other diagnostic gated on this flag).
    if (equalc(tok, "{") && mem->ty->kind != TY_STRUCT && mem->ty->kind != TY_UNION &&
        mem->ty->kind != TY_ARRAY && !in_speculative_const_fold && in_constexpr_init)
        warn_tok(tok, "braces around scalar initializer");
    return global_init_one(tok, var, mem->ty, base_offset + mem->offset);
}

// Initialize one object of type `ty` at `base + offset` in global init data.
// Handles scalars, arrays, structs, compound literals, and flattened init.
static Token *global_init_one(Token *tok, LVar *var, Type *ty, int offset) {
    // "{ STRLIT }" / "{ STRLIT, }" for a char/wide-char array target is a
    // superfluous-but-legal single-element brace, exactly equivalent to
    // the bare "STRLIT" form the two checks just below look for. Unwrap
    // it here so they see it either way — otherwise it falls into the
    // generic "array with braces" per-element handler further down,
    // which parses the string as one scalar element of ty->base and
    // silently produces garbage for whatever element type it doesn't
    // happen to fit (no diagnostic for e.g. a mismatched-width wide
    // string, and wrong bytes even for a same-width match).
    // Only applies to an array whose element is itself scalar (char-like
    // or wide-char-like) — a multi-dimensional array's element is another
    // array (e.g. char[2][3]'s element is char[3]), and "{ "ab" }" there
    // is the standard brace-elision idiom for initializing just the first
    // row, correctly handled by the generic "array with braces" per-
    // element recursion below (which hands "ab" to *that* inner array,
    // where these same checks apply again).
    // TY_PTR is also excluded: a single-element "{ "str" }" initializer for
    // an array of pointers (e.g. "const char *strs[] = { "a" };") assigns
    // the string's address to strs[0] — a completely different, valid case
    // from a char/wide-char array being initialized BY a string literal,
    // not one array-element-type/string-width mismatch.
    bool scalarish_base = ty->kind == TY_ARRAY && ty->base->kind != TY_ARRAY &&
        ty->base->kind != TY_STRUCT && ty->base->kind != TY_UNION &&
        ty->base->kind != TY_PTR;
    Token *brace_close = NULL;
    // A parenthesized string-literal chain — `char s[] = ( "a" "b" )` — is
    // still a string literal (C11 6.7.9p14), e.g. diffutils'
    // C_ifdef_group_formats. Unwrap the parens so the TK_STR branch below
    // sees the chain (adjacent strings concatenate in the expression
    // parser), and skip past the closing paren afterward.
    if (scalarish_base && equalc(tok, "(") && tok->next && tok->next->kind == TK_STR) {
        Token *t = tok->next;
        while (t && t->kind == TK_STR)
            t = t->next;
        if (equalc(t, ")")) {
            brace_close = t->next;
            tok = tok->next;
        }
    }
    if (scalarish_base && equalc(tok, "{") && tok->next && tok->next->kind == TK_STR) {
        Token *after = tok->next->next;
        if (equalc(after, ",")) after = after->next;
        if (equalc(after, "}")) {
            brace_close = after->next;
            tok = tok->next; // unwrap: point straight at the string literal
        }
    }
    // String literal for char/char8_t array
    // A parenthesized string-literal chain initializer — `char s[] =
    // ( "a" "b" )` — is still a string literal (C11 6.7.9p14), e.g.
    // diffutils' C_ifdef_group_formats. Concatenate the inner strings and
    // write their bytes, then skip past the closing paren.
    if (ty->kind == TY_ARRAY && ty->base->kind == TY_CHAR && equalc(tok, "(")) {
        Token *t = tok->next;
        int total = 0;
        Token *first = NULL;
        Token *last = NULL;
        for (; t && t->kind == TK_STR; t = t->next) {
            if (!first) first = t;
            last = t;
            total += t->len;
        }
        if (first && equalc(t, ")")) {
            // Concat all inner string contents (they are already
            // NUL-free decoded buffers) plus the terminator.
            int len = total + 1;
            if (ty->size > 0 && len > ty->size) len = ty->size;
            char *buf = arena_alloc(len);
            int pos = 0;
            for (Token *u = first; u != last->next && pos < len; u = u->next) {
                int n = u->len;
                if (pos + n > len) n = len - pos;
                memcpy(buf + pos, u->str, n);
                pos += n;
            }
            if (pos < len) buf[pos] = 0;
            ensure_init_size(var, offset, len);
            memcpy(var->init_data + offset, buf, len);
            return t->next;
        }
    }

    if (ty->kind == TY_ARRAY && ty->base->kind == TY_CHAR && tok->kind == TK_STR &&
        (tok->string_literal_prefix == 0 || tok->string_literal_prefix == '8')) {
        int len = tok->len + 1; // include embedded NULs and the terminator
        if (ty->size > 0 && len > ty->size) len = ty->size;
        ensure_init_size(var, offset, len);
        memcpy(var->init_data + offset, tok->str, len);
        return brace_close ? brace_close : tok->next;
    }

    // Wide string literal L"..."/u"..."/U"..." for wchar_t[]/char16_t[]/
    // char32_t[]; the writer adapts to the element size (2 or 4 bytes)
    if (ty->kind == TY_ARRAY && tok->kind == TK_STR &&
        (tok->string_literal_prefix == 'L' || tok->string_literal_prefix == 'u' ||
         tok->string_literal_prefix == 'U') &&
        (ty->base->size == 4 || ty->base->size == 2)) {
        int wchar_size = ty->base->size;
        // Decode UTF-8 to wchar_t codepoints
        char *p = tok->str;
        char *end = p + tok->len;
        int max_chars = (ty->size > 0) ? (ty->size / wchar_size) : 0x7fffffff;
        int i = 0;
        while (p < end && i < max_chars - 1) {
            char *next_p = p;
            uint32_t cp = decode_utf8(&next_p, p);
            p = next_p;
            ensure_init_size(var, offset + i * wchar_size, wchar_size);
            if (wchar_size == 4) {
                uint32_t wc = cp;
                memcpy(var->init_data + offset + i * wchar_size, &wc, 4);
            } else {
                uint16_t wc = (uint16_t)cp;
                memcpy(var->init_data + offset + i * wchar_size, &wc, 2);
            }
            i++;
        }
        // Null terminator
        if (i < max_chars) {
            ensure_init_size(var, offset + i * wchar_size, wchar_size);
            memset(var->init_data + offset + i * wchar_size, 0, wchar_size);
        }
        // If array size is 0 (incomplete), set it
        if (ty->size == 0)
            ty->size = (int64_t)(i + 1) * wchar_size;
        return brace_close ? brace_close : tok->next;
    }

    // A string literal directly at an array target (bare, or just
    // unwrapped from "{ ... }" above) whose prefix is incompatible with
    // the target's element type/width — e.g. u8"..." (itself an array
    // of unsigned char per the standard) assigned to a char16_t/
    // char32_t/wchar_t array, or an L/u/U-prefixed literal assigned to
    // a plain char/char8_t array — matched neither branch above. This is
    // a real constraint violation; without this check it fell through to
    // the generic per-element "array with braces" handling (for the
    // unwrapped case) or the address-only extract_reloc() fallback much
    // further below, neither of which validates element-type/width
    // compatibility at all.
    if (scalarish_base && tok->kind == TK_STR) {
        error_tok(tok, "initializing an array of incompatible element type "
                       "with a string literal");
    }

    // Array with braces: { elem1, elem2, ... } with optional [N]=val or [N...M]=val designators
    if (ty->kind == TY_ARRAY && equalc(tok, "{")) {
        int elem_size = ty->base->size;
        int len = array_len(ty);
        tok = skip(tok, "{");
        int idx = 0;
        while (!equalc(tok, "}")) {
            int sidx = idx, eidx = idx;
            if (equalc(tok, "[")) {
                tok = tok->next;
                Node *n = assign(&tok, tok);
                if (n->ty && n->ty->kind == TY_NULLPTR_T)
                    error_tok(n->tok, "array designator is not of integer type");
                long long sv = 0;
                eval_const_expr(n, &sv);
                sidx = (int)sv;
                eidx = sidx;
                if (equalc(tok, "...")) {
                    tok = tok->next;
                    Node *n2 = assign(&tok, tok);
                    long long ev = sidx;
                    eval_const_expr(n2, &ev);
                    eidx = (int)ev;
                }
                tok = skip(tok, "]");
                /* Nested designator: [N][M]=val for multi-dimensional arrays */
                if (equalc(tok, "[")) {
                    tok = tok->next;
                    Node *n2 = assign(&tok, tok);
                    long long sv2 = 0;
                    eval_const_expr(n2, &sv2);
                    int sidx2 = (int)sv2;
                    tok = skip(tok, "]");
                    tok = skip(tok, "=");
                    /* Apply value to a[sidx][sidx2] */
                    if (len == 0 || sidx < len)
                        tok = global_init_one(tok, var, ty->base->base, offset + sidx * elem_size + sidx2 * ty->base->base->size);
                    else
                        tok = skip_initializer(tok);
                    idx = sidx + 1;
                    if (equalc(tok, ",")) {
                        tok = tok->next;
                        if (equalc(tok, "}"))
                            break;
                        continue;
                    }
                    break;
                }
                // Combined designator: [N].member[.sub]* = val (C99 6.7.8p17)
                // for an array of struct/union elements, e.g.
                // drivers/gpu/drm/i915/gt/intel_engine_cs.c's
                // "[RENDER_CLASS].reg = GEN8_RTCR" — a member-name
                // designator directly on one array element instead of a
                // full brace-enclosed "{ .reg = ... }" value.
                if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT &&
                    ty->base && (ty->base->kind == TY_STRUCT || ty->base->kind == TY_UNION)) {
                    int chain_base = offset + sidx * elem_size;
                    Type *cur_ty = ty->base;
                    while (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                        char *sname = tok->next->name;
                        tok = tok->next->next;
                        Member *sm = find_member_by_name(cur_ty, sname);
                        if (!sm) {
                            tok = skip_initializer(tok);
                            break;
                        }
                        if (!equalc(tok, ".")) {
                            tok = skip(tok, "=");
                            tok = global_init_member(tok, var, sm, chain_base);
                            break;
                        }
                        chain_base += sm->offset;
                        cur_ty = sm->ty;
                    }
                    idx = sidx + 1;
                    if (equalc(tok, ",")) {
                        tok = tok->next;
                        if (equalc(tok, "}"))
                            break;
                        continue;
                    }
                    break;
                }
                tok = skip(tok, "=");
            }
            Token *val_start = tok;
            for (int i = sidx; i <= eidx; i++) {
                if (len == 0 || i < len)
                    tok = global_init_one(val_start, var, ty->base, offset + i * elem_size);
                else
                    tok = skip_initializer(val_start);
            }
            idx = eidx + 1;
            if (equalc(tok, ",")) {
                tok = tok->next;
                if (equalc(tok, "}"))
                    break;
                continue;
            }
            break;
        }
        return skip(tok, "}");
    }

    // Struct/union with braces: { mem1, mem2, ... }
    if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && equalc(tok, "{")) {
        tok = skip(tok, "{");
        Member *mem = ty->members;
        while (!equalc(tok, "}")) {
            // Designated initializer: .member[.sub]* = value
            if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                char *name = tok->next->name;
                tok = tok->next->next;
                Member *m = find_member_by_name(ty, name);
                if (m && (equalc(tok, ".") || equalc(tok, "["))) {
                    // Nested designator .f.d[.e]* = value: follow chain
                    int chain_base = offset + m->offset;
                    Type *cur_ty = m->ty;
                    while ((equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) ||
                           equalc(tok, "[")) {
                        if (equalc(tok, "[")) {
                            // Array index designator: [N] or range [N ... M]
                            // (GNU extension; terminal only), e.g.
                            // opcodes/i386-dis.c's struct-member
                            // array-range designators.
                            tok = tok->next;
                            long long idx = 0;
                            if (!equalc(tok, "]")) {
                                Node *idx_node = conditional(&tok, tok);
                                check_type(idx_node);
                                if (!eval_const_expr(idx_node, &idx))
                                    error_tok(tok, "expected constant expression for array index");
                            }
                            long long eidx = idx;
                            bool is_range = false;
                            if (equalc(tok, "...")) {
                                is_range = true;
                                tok = tok->next;
                                Node *idx2 = conditional(&tok, tok);
                                check_type(idx2);
                                if (!eval_const_expr(idx2, &eidx))
                                    error_tok(tok, "expected constant expression for array range designator");
                            }
                            tok = skip(tok, "]");
                            if (cur_ty->kind != TY_ARRAY)
                                error_tok(tok, "array index designator for non-array type");
                            int elem_size = cur_ty->base->size;
                            if (is_range) {
                                tok = skip(tok, "=");
                                Type *elem_ty = cur_ty->base;
                                Token *val_start = tok;
                                for (long long i = idx; i <= eidx; i++)
                                    tok = global_init_one(val_start, var, elem_ty, chain_base + (int)(i * elem_size));
                                break;
                            }
                            chain_base += (int)(idx * elem_size);
                            cur_ty = cur_ty->base;
                            // Chain ends on an array index, e.g.
                            // ".extent[0] = { ... }": consume "=" and parse
                            // the value here too, the same way the
                            // ".member" step below does — otherwise the
                            // loop exits (tok is "=", not "." or "[")
                            // without ever consuming the value, leaving
                            // "= ..." to be misparsed as the next
                            // designator/member.
                            if (!equalc(tok, ".") && !equalc(tok, "[")) {
                                tok = skip(tok, "=");
                                tok = global_init_one(tok, var, cur_ty, chain_base);
                                break;
                            }
                            continue;
                        }
                        // .member designator
                        char *sname = tok->next->name;
                        tok = tok->next->next;
                        Member *sm = find_member_by_name(cur_ty, sname);
                        if (!sm) {
                            tok = skip_initializer(tok);
                            break;
                        }
                        if (!equalc(tok, ".") && !equalc(tok, "[")) {
                            tok = skip(tok, "=");
                            tok = global_init_member(tok, var, sm, chain_base);
                            break;
                        }
                        chain_base += sm->offset;
                        cur_ty = sm->ty;
                    }
                    mem = m->next;
                } else {
                    tok = skip(tok, "=");
                    if (m) {
                        tok = global_init_member(tok, var, m, offset);
                        mem = m->next;
                    } else {
                        tok = skip_initializer(tok);
                    }
                }
                // Old-style GNU designator: member: value (without leading '.')
            } else if (tok->kind == TK_IDENT && equalc(tok->next, ":")) {
                char *name = tok->name;
                tok = tok->next->next; // skip "member" ":"
                Member *m = find_member_by_name(ty, name);
                if (m) {
                    tok = global_init_member(tok, var, m, offset);
                    mem = m->next;
                } else {
                    tok = skip_initializer(tok);
                }
            } else if (mem) {
                tok = global_init_member(tok, var, mem, offset);
                mem = mem->next;
            } else {
                tok = skip_initializer(tok);
            }
            if (equalc(tok, ",")) {
                tok = tok->next;
                if (equalc(tok, "}"))
                    break;
                continue;
            }
            break;
        }
        return skip(tok, "}");
    }

    // Compound literal for aggregate type
    if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION || ty->kind == TY_ARRAY) && find_compound_literal_start(tok)) {
        Token *compound_start = find_compound_literal_start(tok);
        tok = skip(compound_start, "{");
        if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
            Member *mem = ty->members;
            while (!equalc(tok, "}")) {
                if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                    char *name = tok->next->name;
                    tok = tok->next->next;
                    tok = skip(tok, "=");
                    Member *m = find_member_by_name(ty, name);
                    if (m) {
                        tok = global_init_member(tok, var, m, offset);
                        mem = m->next;
                    } else {
                        tok = skip_initializer(tok);
                    }
                } else if (mem) {
                    tok = global_init_member(tok, var, mem, offset);
                    mem = mem->next;
                } else {
                    tok = skip_initializer(tok);
                }
                if (equalc(tok, ",")) {
                    tok = tok->next;
                    if (equalc(tok, "}"))
                        break;
                    continue;
                }
                break;
            }
        } else { // array
            int elem_size = ty->base->size;
            int len = array_len(ty);
            int idx = 0;
            while (!equalc(tok, "}")) {
                if (len == 0 || idx < len)
                    tok = global_init_one(tok, var, ty->base, offset + idx * elem_size);
                else
                    tok = skip_initializer(tok);
                idx++;
                if (equalc(tok, ",")) {
                    tok = tok->next;
                    if (equalc(tok, "}"))
                        break;
                    continue;
                }
                break;
            }
        }
        if (equalc(tok, "}"))
            tok = tok->next;
        while (equalc(tok, ")"))
            tok = tok->next;
        return tok;
    }

    // If initializing struct/union from a constexpr var, copy its init_data
    // Search both globals and local constexpr variables.
    if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && tok->kind == TK_IDENT) {
        LVar *src = find_global_name(tok->name);
        if (!src || !src->is_constexpr || !src->init_data) {
            // Try local constexpr variables
            for (LVar *lv = locals; lv; lv = lv->next) {
                if (lv->name == tok->name && lv->is_constexpr && lv->init_data) {
                    src = lv;
                    break;
                }
            }
        }
        if (src && src->is_constexpr && src->init_data) {
            ensure_init_size(var, offset, ty->size);
            memcpy(var->init_data + offset, src->init_data, ty->size);
            var->has_init = true;
            return tok->next;
        }
    }
    // If initializing struct/union from a single constexpr var expression, copy its init_data
    if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && tok->kind == TK_IDENT) {
        LVar *src = find_global_name(tok->name);
        if (src && src->is_constexpr && src->init_data) {
            ensure_init_size(var, offset, ty->size);
            memcpy(var->init_data + offset, src->init_data, ty->size);
            var->has_init = true;
            return tok->next;
        }
    }
    // If initializing struct/union from a single constexpr var expression, copy its init_data
    if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && tok->kind == TK_IDENT) {
        LVar *src = find_global_name(tok->name);
        if (src && src->is_constexpr && src->init_data) {
            ensure_init_size(var, offset, ty->size);
            memcpy(var->init_data + offset, src->init_data, ty->size);
            var->has_init = true;
            return tok->next;
        }
    }
    // If initializing struct/union from a single constexpr var expression, copy its init_data
    if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && tok->kind == TK_IDENT) {
        LVar *src = find_global_name(tok->name);
        if (src && src->is_constexpr && src->init_data) {
            ensure_init_size(var, offset, ty->size);
            memcpy(var->init_data + offset, src->init_data, ty->size);
            var->has_init = true;
            return tok->next;
        }
    }
    // Struct/union without braces: flatten into members.
    // For unions, only the first member is initialized.
    if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
        Member *mem = ty->members;
        if (mem) {
            tok = global_init_member(tok, var, mem, offset);
            mem = mem->next;
            if (ty->kind == TY_STRUCT) {
                while (mem && !equalc(tok, "}")) {
                    if (equalc(tok, ","))
                        tok = tok->next;
                    if (equalc(tok, "}"))
                        break;
                    tok = global_init_member(tok, var, mem, offset);
                    mem = mem->next;
                }
            }
        }
        return tok;
    }

    // Array without braces: single element
    if (ty->kind == TY_ARRAY) {
        return global_init_one(tok, var, ty->base, offset);
    }

    // Superfluous braces around scalar `{ expr }`, or C23 empty init `{}`.
    if (equalc(tok, "{")) {
        tok = skip(tok, "{");
        if (!equalc(tok, "}")) { // `{}` leaves the (already zeroed) storage as 0
            tok = global_init_one(tok, var, ty, offset);
            // C11 6.7.9p11's "single expression, optionally enclosed in
            // braces" explicitly permits a trailing comma inside those
            // braces -- e.g. a designated struct-member initializer like
            // `.specs = { "refs/heads/master", }`.
            if (equalc(tok, ","))
                tok = tok->next;
        }
        tok = skip(tok, "}");
        return tok;
    }

    // Pointer to label/function
    if (ty->kind == TY_PTR) {
        char *label = NULL;
        int addend = 0;
        Token *next = tok;
        if (read_global_label_initializer(&next, tok, &label, &addend)) {
            append_reloc(var, offset, label, addend);
            return next;
        }
        // &(compound literal) for pointer types. GCC-style macros commonly
        // wrap the whole thing in one extra redundant paren —
        // "(&(type){...})", e.g. linux/hwmon.h's HWMON_CHANNEL_INFO() —
        // which a bare equalc(tok, "&") check misses entirely (tok is "("
        // here, not "&"); peel off that one layer first if present.
        //
        // A genuine CAST in front of "&(compound literal)" -- e.g.
        // "(void *) &(T){...}" -- reinterprets the literal's address as a
        // different pointer type; njs's njs_symval()/njs_ascii_strval()
        // macros nest exactly this shape ("(void*) &(njs_value_t){...}").
        // The address (and thus the relocation) is unaffected by the
        // cast, so skip over any chain of leading casts first -- the
        // existing wrapped_amp/bare-"&" detection below never looked past
        // a leading "(" it couldn't itself resolve to "&".
        while (equalc(tok, "(") && is_typename(tok->next)) {
            Token *t2 = tok->next;
            int depth = 1;
            while (t2 && depth > 0) {
                if (equalc(t2, "(")) depth++;
                else if (equalc(t2, ")"))
                    depth--;
                if (depth > 0) t2 = t2->next;
            }
            if (!t2 || !equalc(t2, ")")) break;
            Token *after = t2->next;
            if (!(equalc(after, "&") && find_compound_literal_start(after->next)))
                break;
            tok = after;
        }
        bool wrapped_amp = equalc(tok, "(") && equalc(tok->next, "&") &&
            find_compound_literal_start(tok->next->next);
        Token *amp_tok = wrapped_amp ? tok->next : tok;
        if (equalc(amp_tok, "&") && find_compound_literal_start(amp_tok->next)) {
            tok = amp_tok->next;
            Token *compound_start = find_compound_literal_start(tok);
            Token *t = tok;
            int open_count = 0;
            while (equalc(t, "(")) {
                t = t->next;
                open_count++;
            }
            for (Token *u = t; u && !equalc(u, ")"); u = u->next)
                if (equalc(u, "register") || equalc(u, "thread_local") || equalc(u, "_Thread_local"))
                    error_tok(u, "file-scope compound literal specifies storage class");
            bool saved_icl = in_compound_literal;
            in_compound_literal = true;
            Type *compound_ty = type_name(&t, t);
            in_compound_literal = saved_icl;
            int close_count = 0;
            while (equalc(t, ")")) {
                t = t->next;
                close_count++;
            }
            static int anon_count;
            char *name = format(".Lanon.%d", anon_count++);
            LVar *anon_var = new_var(name, compound_ty, false);
            // Compiler-synthesized, invisible-outside-this-file temporary:
            // must be a LOCAL (not GLOBAL) ELF symbol, or two separate
            // translation units each independently counting from
            // ".Lanon.0" collide at link time ("multiple definition of
            // '.Lanon.0'") — every compilation unit resets anon_count to 0.
            anon_var->is_static = true;
            Token *rest_inner = NULL;
            global_initializer(&rest_inner, compound_start, anon_var);
            tok = rest_inner;
            // Any extra redundant parens wrapped directly around the
            // compound literal itself (open_count counts both those and
            // the type-cast's own required paren, already closed above)
            // close here, immediately after the literal's initializer body.
            for (int i = 0; i < open_count - close_count; i++)
                tok = skip(tok, ")");
            // A trailing chain of constant subscripts/member accesses may
            // still follow — e.g. "&(...)[0].attr.attr" — fold it into the
            // relocation's addend instead of leaving it unparsed.
            int chain_addend = parse_const_addend_chain(&tok, tok, compound_ty);
            if (wrapped_amp) tok = skip(tok, ")");
            append_reloc(var, offset, name, chain_addend);
            return tok;
        }
        // Bare (no leading &) array-typed compound literal assigned to a
        // pointer field: array-to-pointer decay, e.g. linux/hwmon.h's
        // HWMON_CHANNEL_INFO(): ".config = (const u32 []) { A, B, 0 }"
        // where .config's declared type is "const u32 *". Materializes
        // the array as its own anonymous static object (recursing through
        // global_initializer exactly like the "&(compound literal)" case
        // above) and points this pointer field at its first element —
        // same reloc mechanism, just without an explicit "&" and with the
        // compound literal's type being the array itself rather than the
        // struct/scalar the pointer's target type would suggest. Any
        // redundant wrapping paren is already handled by
        // find_compound_literal_start()'s own leading-"(" skip loop —
        // no separate unwrap step needed here (unlike the "&(...)" case
        // above, which checks for a literal leading "&" itself).
        if (find_compound_literal_start(tok)) {
            Token *compound_start = find_compound_literal_start(tok);
            Token *t = tok;
            int open_count = 0;
            while (equalc(t, "(")) {
                t = t->next;
                open_count++;
            }
            for (Token *u = t; u && !equalc(u, ")"); u = u->next)
                if (equalc(u, "register") || equalc(u, "thread_local") || equalc(u, "_Thread_local"))
                    error_tok(u, "file-scope compound literal specifies storage class");
            bool saved_icl = in_compound_literal;
            in_compound_literal = true;
            Type *compound_ty = type_name(&t, t);
            in_compound_literal = saved_icl;
            int close_count = 0;
            while (equalc(t, ")")) {
                t = t->next;
                close_count++;
            }
            if (compound_ty->kind == TY_ARRAY) {
                static int anon_arr_count;
                char *name = format(".Lanonarr.%d", anon_arr_count++);
                LVar *anon_var = new_var(name, compound_ty, false);
                anon_var->is_static = true; // see the identical comment above
                Token *rest_inner = NULL;
                global_initializer(&rest_inner, compound_start, anon_var);
                tok = rest_inner;
                // Redundant parens wrapped directly around the compound
                // literal itself close here, immediately after its body —
                // e.g. drivers/gpu/drm/i915/display/intel_display_power_map.c's
                // "((const struct instance[]) { ... })" nested inside a
                // wider designated initializer.
                for (int i = 0; i < open_count - close_count; i++)
                    tok = skip(tok, ")");
                // A trailing chain of constant subscripts/member accesses
                // may still follow — fold it into the relocation's addend.
                int chain_addend = parse_const_addend_chain(&tok, tok, compound_ty);
                append_reloc(var, offset, name, chain_addend);
                return tok;
            }
        }
    }

    // Scalar
    Node *node = assign(&tok, tok);
    check_type(node);

    // For pointer types, try extracting a reloc from the expression
    if (ty->kind == TY_PTR) {
        char *label = NULL;
        int addend = 0;
        if (extract_reloc(node, &label, &addend)) {
            if (label)
                append_reloc(var, offset, label, addend);
            else
                write_scalar_bytes(var, offset, ty->size, (int64_t)addend);
            return tok;
        }
    }

    if (is_complex(ty)) {
        double rv = 0.0, iv = 0.0;
        if (eval_complex_const_expr(node, &rv, &iv)) {
            int base_sz = ty->base ? ty->base->size : 4;
            ensure_init_size(var, offset, base_sz * 2);
            if (is_flonum(ty->base)) {
                if (base_sz == 4) {
                    float rf = (float)rv, imf = (float)iv;
                    memcpy(var->init_data + offset, &rf, 4);
                    memcpy(var->init_data + offset + 4, &imf, 4);
                } else {
                    // base_sz 8 (double) or 16 (long double, stored as a
                    // double payload at the start of its 16-byte slot): the
                    // imaginary part always sits at offset base_sz.
                    memcpy(var->init_data + offset, &rv, 8);
                    memcpy(var->init_data + offset + base_sz, &iv, 8);
                }
            } else {
                write_scalar_bytes(var, offset, base_sz, (int64_t)rv);
                write_scalar_bytes(var, offset + base_sz, base_sz, (int64_t)iv);
            }
            return tok;
        }
        if (!var->is_local && !in_speculative_const_fold)
            error_tok(tok, "expected constant expression in initializer");
        else if (in_speculative_const_fold)
            speculative_fold_failed = true;
        return tok;
    }
    if (is_flonum(ty)) {
        double fv = 0;
        if (eval_double_const_expr(node, &fv)) {
            if (ty->size == 4) {
                float f = (float)fv;
                memcpy(var->init_data + offset, &f, 4);
            } else {
                memcpy(var->init_data + offset, &fv, 8);
            }
            return tok;
        }
        if (!var->is_local && !in_speculative_const_fold)
            error_tok(tok, "expected constant expression in initializer");
        else if (in_speculative_const_fold)
            speculative_fold_failed = true;
        return tok;
    }
    // A relocatable address stored in an integer-typed (not pointer-typed)
    // struct member — e.g. arch/x86/include/asm/processor.h's INIT_THREAD:
    // "{ .sp = (unsigned long)&__top_init_kernel_stack }", where .sp is a
    // plain `unsigned long` field. Mirrors the ty->kind == TY_PTR handling
    // a few lines up; that branch only fires for pointer-typed members, so
    // an integer-typed member holding a cast address never tried
    // extract_reloc() at all.
    //
    // Must run BEFORE the plain eval_const_expr() below: that evaluator
    // folds a string literal (or any address expression) to its truthiness
    // (ND_STR -> 1), never its address, so `(intptr_t)"lit"` / `(unsigned
    // long)&sym` in an integer scalar would silently store 1 instead of a
    // relocation (git's `struct option` tables, `.defval = (intptr_t)"all"`).
    //
    // Only fires when the field is at least pointer-width: a real relocation
    // needs the full 8-byte address to be representable, so on LLP64
    // (Windows, "unsigned long" is 4 bytes) this must NOT fire for `unsigned
    // long` the way it safely can on LP64 — GCC itself rejects "(unsigned
    // long)&sym" as a global initializer on Windows ("initializer element is
    // not constant"; confirmed against real x86_64-w64-mingw32-gcc), for
    // exactly this reason: the address genuinely may not fit. Narrower
    // fields fall through to the existing "unsupported"/"expected constant
    // expression" error below, matching GCC.
    if (ty->size >= 8) {
        char *label = NULL;
        int addend = 0;
        if (looks_like_address_expr(node) && extract_reloc(node, &label, &addend) && label) {
            append_reloc(var, offset, label, addend);
            return tok;
        }
    }
    long long val = 0;
    if (eval_const_expr(node, &val)) {
        write_scalar_bytes(var, offset, ty->size, (int64_t)val);
        return tok;
    }
    // Label-address DIFFERENCE (GCC's `&&label_a - &&label_b` computed-goto
    // jump-table idiom, e.g. torture/pr70460.c's `static int b[] = { &&lab1
    // - &&lab0, &&lab2 - &&lab0 };`). Applies at any integer size (unlike
    // the >=8-byte-only single-address case above): neither label's byte
    // offset is knowable yet — labels live in .text, defined only once the
    // enclosing function's body is generated, strictly after every static
    // initializer in this early pass — so it can never fold via
    // eval_const_expr(); defer it to codegen.c as a label-diff reloc,
    // resolved as a same-object byte patch (there is no ELF/Mach-O
    // relocation kind for "symbol A minus symbol B") once the enclosing
    // function's body has been generated.
    {
        char *label_hi = NULL, *label_lo = NULL;
        if (extract_label_diff(node, &label_hi, &label_lo)) {
            append_label_diff_reloc(var, offset, label_hi, label_lo, ty->size);
            return tok;
        }
    }
    if (!var->is_local && !in_speculative_const_fold)
        error_tok(tok, "expected constant expression in initializer");
    else if (in_speculative_const_fold)
        speculative_fold_failed = true;
    return tok;
}

// Forward declarations for local recursive initializer
static Token *local_init_one(Token *tok, Node *lhs, Type *ty, Node **cur);

static Node *new_array_elem_lvalue_node(Node *base, int idx, Token *tok) {
    Node *offset = new_num(idx, tok);
    Node *add = new_binary(ND_ADD, base, offset, tok);
    check_type(add);
    Node *deref = new_unary(ND_DEREF, add, tok);
    check_type(deref);
    return deref;
}

static Token *local_init_flat_array(Token *tok, Node *lhs, Type *ty, Node **cur) {
    if (ty->kind == TY_ARRAY) {
        int len = array_len(ty);
        Type *base = ty->base;
        for (int i = 0; i < len && !equalc(tok, "}"); i++) {
            Node *elem_lhs = new_array_elem_lvalue_node(lhs, i, tok);
            tok = local_init_flat_array(tok, elem_lhs, base, cur);
            if (i < len - 1 && equalc(tok, ","))
                tok = tok->next;
        }
        return tok;
    }
    return local_init_one(tok, lhs, ty, cur);
}

static Token *local_init_member(Token *tok, Node *lhs, Member *mem, Node **cur) {
    Node *mem_node = new_unary(ND_MEMBER, lhs, tok);
    mem_node->member = mem;
    check_type(mem_node);
    // Same parenthesized-string-literal exception as global_init_member.
    if (mem->ty->kind == TY_ARRAY && !equalc(tok, "{") && tok->kind != TK_STR &&
        !(equalc(tok, "(") && tok->next && tok->next->kind == TK_STR)) {
        return local_init_flat_array(tok, mem_node, mem->ty, cur);
    }
    // See global_init_member's identical check: an extra brace level
    // around a scalar member is redundant nesting, unlike a bare
    // scalar's own optionally-braced initializer.
    if (equalc(tok, "{") && mem->ty->kind != TY_STRUCT && mem->ty->kind != TY_UNION &&
        mem->ty->kind != TY_ARRAY && in_constexpr_init)
        warn_tok(tok, "braces around scalar initializer");
    return local_init_one(tok, mem_node, mem->ty, cur);
}

static Token *local_init_one(Token *tok, Node *lhs, Type *ty, Node **cur) {
    // "{ STRLIT }" / "{ STRLIT, }" for a char/wide-char array target is a
    // superfluous-but-legal single-element brace (C11 6.7.9p14), exactly
    // equivalent to the bare STRLIT form the check just below looks for.
    // Unwrap it here so that check sees it either way -- otherwise it fell
    // into the generic "Array with braces" per-element loop further down,
    // which treated the whole string literal as ONE initializer for
    // ty->base (a scalar char), assigning the string's decayed `char*`
    // address (truncated to one byte) into element 0 and leaving every
    // other element at its zero-initialized default. Mirrors
    // global_init_one()'s identical unwrap (used for static/constexpr
    // locals and true globals) and infer_array_type()'s sizing unwrap --
    // all three must agree on this shape.
    bool scalarish_base = ty->kind == TY_ARRAY && ty->base->kind != TY_ARRAY &&
        ty->base->kind != TY_STRUCT && ty->base->kind != TY_UNION &&
        ty->base->kind != TY_PTR;
    Token *brace_close = NULL;
    // A parenthesized string-literal chain — `char s[] = ( "a" "b" )` — is
    // still a string literal (C11 6.7.9p14), e.g. diffutils'
    // C_ifdef_group_formats. Unwrap the parens so the TK_STR branch below
    // sees the chain (adjacent strings concatenate in the expression
    // parser), and skip past the closing paren afterward.
    if (scalarish_base && equalc(tok, "(") && tok->next && tok->next->kind == TK_STR) {
        Token *t = tok->next;
        while (t && t->kind == TK_STR)
            t = t->next;
        if (equalc(t, ")")) {
            brace_close = t->next;
            tok = tok->next;
        }
    }
    if (scalarish_base && equalc(tok, "{") && tok->next && tok->next->kind == TK_STR) {
        Token *after = tok->next->next;
        if (equalc(after, ",")) after = after->next;
        if (equalc(after, "}")) {
            brace_close = after->next;
            tok = tok->next; // unwrap: point straight at the string literal
        }
    }
    // String literal for char or wide-char array
    if (ty->kind == TY_ARRAY && tok->kind == TK_STR &&
        (ty->base->kind == TY_CHAR || tok->string_literal_prefix != 0)) {
        Node *rhs = assign(&tok, tok);
        Node *assign_node = new_binary(ND_ASSIGN, lhs, rhs, tok);
        check_type(assign_node);
        *cur = (*cur)->next = new_unary(ND_EXPR_STMT, assign_node, tok);
        return brace_close ? brace_close : tok;
    }

    // Array with braces
    if (ty->kind == TY_ARRAY && equalc(tok, "{")) {
        int len = array_len(ty);
        tok = skip(tok, "{");
        int idx = 0;
        while (!equalc(tok, "}")) {
            int sidx = idx, eidx = idx;
            if (equalc(tok, "[")) {
                tok = tok->next;
                Node *n = assign(&tok, tok);
                if (n->ty && n->ty->kind == TY_NULLPTR_T)
                    error_tok(n->tok, "array designator is not of integer type");
                long long sv = 0;
                eval_const_expr(n, &sv);
                sidx = (int)sv;
                eidx = sidx;
                if (equalc(tok, "...")) {
                    tok = tok->next;
                    Node *n2 = assign(&tok, tok);
                    long long ev = sidx;
                    eval_const_expr(n2, &ev);
                    eidx = (int)ev;
                }
                tok = skip(tok, "]");
                /* Nested designator: [N][M]=val for multi-dimensional arrays */
                if (equalc(tok, "[")) {
                    Node *inner = new_array_elem_lvalue_node(lhs, sidx, tok);
                    tok = tok->next;
                    Node *n2 = assign(&tok, tok);
                    long long sv2 = 0;
                    eval_const_expr(n2, &sv2);
                    int sidx2 = (int)sv2;
                    tok = skip(tok, "]");
                    tok = skip(tok, "=");
                    Node *elem_lhs = new_array_elem_lvalue_node(inner, sidx2, tok);
                    tok = local_init_one(tok, elem_lhs, ty->base->base, cur);
                    idx = sidx + 1;
                    if (equalc(tok, ",")) {
                        tok = tok->next;
                        if (equalc(tok, "}"))
                            break;
                        continue;
                    }
                    break;
                }
                tok = skip(tok, "=");
            }
            Token *val_start = tok;
            if (sidx != eidx && !equalc(tok, "{")) {
                // Range with scalar/non-brace value: evaluate once into a temp
                Token *after_val = tok;
                Node *rhs = assign(&after_val, after_val);
                check_type(rhs);
                LVar *tmp = new_var("", rhs->ty, true);
                Node *tmp_assign = new_binary(ND_ASSIGN, new_var_node(tmp, tok), rhs, tok);
                check_type(tmp_assign);
                *cur = (*cur)->next = new_unary(ND_EXPR_STMT, tmp_assign, tok);
                for (int i = sidx; i <= eidx; i++) {
                    if (len == 0 || i < len) {
                        Node *elem_lhs = new_array_elem_lvalue_node(lhs, i, tok);
                        Node *assign_node = new_binary(ND_ASSIGN, elem_lhs,
                                                       new_var_node(tmp, tok), tok);
                        check_type(assign_node);
                        *cur = (*cur)->next = new_unary(ND_EXPR_STMT, assign_node, tok);
                    }
                }
                tok = after_val;
            } else {
                for (int i = sidx; i <= eidx; i++) {
                    if (len == 0 || i < len) {
                        Node *elem_lhs = new_array_elem_lvalue_node(lhs, i, tok);
                        tok = local_init_one(val_start, elem_lhs, ty->base, cur);
                    } else {
                        tok = skip_initializer(val_start);
                    }
                }
            }
            idx = eidx + 1;
            if (equalc(tok, ",")) {
                tok = tok->next;
                if (equalc(tok, "}"))
                    break;
                continue;
            }
            break;
        }
        return skip(tok, "}");
    }

    // Struct/union with braces
    if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && equalc(tok, "{")) {
        tok = skip(tok, "{");
        Member *mem = ty->members;
        while (!equalc(tok, "}")) {
            if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                // Parse chain of .member designators
                Node *chain_lhs = lhs;
                Type *chain_ty = ty;
                Member *first_dm = NULL;
                Member *last_dm = NULL;
                bool chain_ok = true;
                bool range_handled = false;
                while ((equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) ||
                       equalc(tok, "[")) {
                    if (equalc(tok, "[")) {
                        // Array-index step in a designator chain: .arr[idx]...
                        // or the GNU range form .arr[LOW ... HIGH] = val
                        // (C99 6.7.8p17-adjacent GNU extension; terminal
                        // only -- sets every index in [LOW,HIGH] to the
                        // same value), e.g. opcodes/i386-dis.c's
                        // struct-member array-range designators.
                        Token *lb = tok;
                        Node *idx = expr(&tok, tok->next);
                        if (chain_ty->kind != TY_ARRAY && chain_ty->kind != TY_PTR) {
                            chain_ok = false;
                            break;
                        }
                        if (equalc(tok, "...")) {
                            long long sidx = 0, eidx = 0;
                            if (!eval_const_expr(idx, &sidx))
                                error_tok(lb, "expected constant expression for array range designator");
                            tok = tok->next; // skip "..."
                            Node *idx2 = assign(&tok, tok);
                            eidx = sidx;
                            if (!eval_const_expr(idx2, &eidx))
                                error_tok(lb, "expected constant expression for array range designator");
                            tok = skip(tok, "]");
                            tok = skip(tok, "=");
                            Type *elem_ty = chain_ty->base;
                            Token *val_start = tok;
                            if (!equalc(tok, "{")) {
                                // Evaluate the value once into a temp so a
                                // side-effecting RHS isn't re-run per index.
                                Token *after_val = tok;
                                Node *rhs = assign(&after_val, after_val);
                                check_type(rhs);
                                LVar *tmp = new_var("", rhs->ty, true);
                                Node *tmp_assign = new_binary(ND_ASSIGN, new_var_node(tmp, tok), rhs, tok);
                                check_type(tmp_assign);
                                *cur = (*cur)->next = new_unary(ND_EXPR_STMT, tmp_assign, tok);
                                for (long long i = sidx; i <= eidx; i++) {
                                    Node *elem_lhs = new_array_elem_lvalue_node(chain_lhs, (int)i, tok);
                                    Node *assign_node = new_binary(ND_ASSIGN, elem_lhs, new_var_node(tmp, tok), tok);
                                    check_type(assign_node);
                                    *cur = (*cur)->next = new_unary(ND_EXPR_STMT, assign_node, tok);
                                }
                                tok = after_val;
                            } else {
                                for (long long i = sidx; i <= eidx; i++) {
                                    Node *elem_lhs = new_array_elem_lvalue_node(chain_lhs, (int)i, tok);
                                    tok = local_init_one(val_start, elem_lhs, elem_ty, cur);
                                }
                            }
                            range_handled = true;
                            break;
                        }
                        tok = skip(tok, "]");
                        Node *sub = new_unary(ND_DEREF,
                                              new_binary(ND_ADD, chain_lhs, idx, lb), lb);
                        check_type(sub);
                        chain_lhs = sub;
                        chain_ty = sub->ty;
                        continue;
                    }
                    char *dname = tok->next->name;
                    tok = tok->next->next;
                    Member *dm = find_member_by_name(chain_ty, dname);
                    if (!dm) {
                        chain_ok = false;
                        break;
                    }
                    if (!first_dm) first_dm = dm;
                    Node *mem_node = new_unary(ND_MEMBER, chain_lhs, tok);
                    mem_node->member = dm;
                    check_type(mem_node);
                    chain_lhs = mem_node;
                    last_dm = dm;
                    chain_ty = dm->ty;
                }
                if (range_handled) {
                    mem = first_dm ? first_dm->next : NULL;
                } else {
                    tok = skip(tok, "=");
                    if (!chain_ok || !last_dm) {
                        tok = skip_initializer(tok);
                    } else {
                        tok = local_init_one(tok, chain_lhs, chain_ty, cur);
                    }
                    mem = first_dm ? first_dm->next : NULL;
                }
            } else if (tok->kind == TK_IDENT && tok->next && equalc(tok->next, ":")) {
                // GNU-style designated init: member: value
                char *name = tok->name;
                tok = tok->next->next;
                Member *m = find_member_by_name(ty, name);
                if (m) {
                    tok = local_init_member(tok, lhs, m, cur);
                    mem = m->next;
                } else {
                    tok = skip_initializer(tok);
                }
            } else if (mem) {
                tok = local_init_member(tok, lhs, mem, cur);
                mem = mem->next;
            } else {
                tok = skip_initializer(tok);
            }
            if (equalc(tok, ",")) {
                tok = tok->next;
                if (equalc(tok, "}"))
                    break;
                continue;
            }
            break;
        }
        return skip(tok, "}");
    }

    // Compound literal for aggregate type
    if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION || ty->kind == TY_ARRAY) && find_compound_literal_start(tok)) {
        Node *rhs = assign(&tok, tok);
        Node *assign_node = new_binary(ND_ASSIGN, lhs, rhs, tok);
        check_type(assign_node);
        *cur = (*cur)->next = new_unary(ND_EXPR_STMT, assign_node, tok);
        return tok;
    }

    // Struct/union without braces: check if single struct expression, else flatten
    if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
        Token *saved = tok;
        Node *node = assign(&saved, saved);
        check_type(node);
        if (node->ty && (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION)) {
            tok = saved;
            Node *assign_node = new_binary(ND_ASSIGN, lhs, node, tok);
            check_type(assign_node);
            *cur = (*cur)->next = new_unary(ND_EXPR_STMT, assign_node, tok);
            return tok;
        }
        // Flatten into members
        Member *mem = ty->members;
        if (mem) {
            tok = local_init_member(tok, lhs, mem, cur);
            mem = mem->next;
            if (ty->kind == TY_STRUCT) {
                while (mem && !equalc(tok, "}")) {
                    if (equalc(tok, ","))
                        tok = tok->next;
                    if (equalc(tok, "}"))
                        break;
                    tok = local_init_member(tok, lhs, mem, cur);
                    mem = mem->next;
                }
            }
        }
        return tok;
    }

    // Array without braces: single element
    if (ty->kind == TY_ARRAY) {
        Node *elem_lhs = new_array_elem_lvalue_node(lhs, 0, tok);
        return local_init_one(tok, elem_lhs, ty->base, cur);
    }

    // Superfluous braces around scalar, or C23 empty initializer `{}`.
    if (equalc(tok, "{")) {
        tok = skip(tok, "{");
        if (equalc(tok, "}")) {
            // C23 `= {}` on a scalar: zero-initialize.
            Node *assign_node = new_binary(ND_ASSIGN, lhs, new_num(0, tok), tok);
            check_type(assign_node);
            *cur = (*cur)->next = new_unary(ND_EXPR_STMT, assign_node, tok);
        } else {
            tok = local_init_one(tok, lhs, ty, cur);
            // C11 6.7.9p11's "single expression, optionally enclosed in
            // braces" explicitly permits a trailing comma inside those
            // braces (6.7.9p19's general trailing-comma allowance is not
            // array/struct-specific) -- e.g. util-linux's isosize.c:
            // `char *specs = { "refs/heads/master", };`.
            if (equalc(tok, ","))
                tok = tok->next;
        }
        tok = skip(tok, "}");
        return tok;
    }

    // Scalar
    Node *rhs = assign(&tok, tok);
    Node *assign_node = new_binary(ND_ASSIGN, lhs, rhs, tok);
    check_type(assign_node);
    *cur = (*cur)->next = new_unary(ND_EXPR_STMT, assign_node, tok);
    return tok;
}

// K&R (old-style) parameter name+type: shared by top-level function
// definitions (parse()) and GNU nested function definitions
// (parse_nested_function_def) - see parse_kr_param_list below.
typedef struct KRParam KRParam;
struct KRParam {
    KRParam *next;
    char *name;
    Type *ty; // NULL until resolved by a matching declaration; defaults to int
    Node *vla_len_expr; // VLA-typed param: size expr to evaluate at each call (C11 6.7.6.3p7 side effects)
};
static KRParam *parse_kr_param_list(Token **rest, Token *tok);

static Token *parse_nested_function_def(Token **rest, Token *tok, Type *fty,
                                        char *decl_name, char *mangled_name, KRParam *kr_params);

// C11 6.2.7p2: two declarations of the same function with incompatible
// types are a constraint violation ("conflicting types"). Shared by the
// file-scope redeclaration path (parse()) and block-scope function
// declarations (declaration()) -- the latter previously never compared
// the local prototype against the file-scope symbol: gnulib's ioctl
// POSIX-signature configure probe declares `int ioctl (int, int, ...);`
// inside main(), which must conflict with glibc's `int ioctl(int,
// unsigned long, ...)`. rcc silently accepted it, gnutls' configure
// concluded the POSIX signature holds, set REPLACE_IOCTL=0 and the
// generated sys/ioctl.h redeclared ioctl with `int request`,
// failing the build (the `# if @SYS_IOCTL_H_HAVE_WINSOCK2_H@ || 1`
// branch). gcc errors on the probe, sets REPLACE_IOCTL=1 and takes the
// rpl_ioctl path -- no conflict.
static bool func_decls_conflict(Type *prev_fty, Type *fty) {
    // Return types must always match (C11 6.2.7p2), old-style parameter
    // lists included: glibc's stdlib.h declares `char *ptsname(int)`
    // under _GNU_SOURCE, and configure probes re-declare it as
    // `int ptsname();` to test whether the declaration is present -- the
    // resulting conflicting-types error is how zsh detects /dev/ptmx
    // support. rcc only compared parameter lists, so the probe compiled
    // clean and zsh took its BSD /dev/ptyXX fallback, which cannot open
    // a pty on Linux.
    if (prev_fty->return_ty && fty->return_ty &&
        !types_compatible_p(prev_fty->return_ty, fty->return_ty) &&
        !(prev_fty->return_ty->is_enum && fty->return_ty->is_enum))
        return true;
    if (!prev_fty->is_oldstyle &&
        ((prev_fty->is_void_params && fty->param_types) ||
         (fty->is_void_params && prev_fty->param_types)))
        return true;
    if (prev_fty->param_types && fty->param_types && !prev_fty->is_oldstyle) {
        if (prev_fty->is_variadic != fty->is_variadic)
            return true;
        Type *pa = prev_fty->param_types;
        Type *pb = fty->param_types;
        while (pa && pb) {
            if (pa->kind != TY_STRUCT && pa->kind != TY_UNION &&
                pb->kind != TY_STRUCT && pb->kind != TY_UNION) {
                // C11 6.7.6.3p10: a parameter's declared qualified type is
                // taken as its UNQUALIFIED version for function-type
                // compatibility -- a top-level const/volatile/restrict on a
                // by-value parameter (including a function-pointer-typed
                // one) differing between a declaration and its definition
                // is NOT a conflict; real gcc/clang accept it silently
                // even under -Wall -Wextra. njs's
                // njs_vm_external_constructor() declares
                // "njs_function_native_t native" but defines
                // "const njs_function_native_t native" -- legal, was
                // previously misdiagnosed here.
                Type ta = *pa, tb = *pb;
                ta.qual = tb.qual = 0;
                if (!type_equal(&ta, &tb))
                    return true;
            }
            pa = pa->param_next;
            pb = pb->param_next;
        }
        if ((pa != NULL) != (pb != NULL))
            return true;
    }
    return false;
}

static Node *declaration(Token **rest, Token *tok) {
    // C23 static_assert / C11 _Static_assert
    if (equalc(tok, "static_assert") || equalc(tok, "_Static_assert")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *cond = conditional(&tok, tok);
        check_type(cond);
        if (cond->ty && !is_integer(cond->ty))
            error_tok(cond->tok, "static_assert condition is not an integer");
        // C11 6.6p6: floating operands only as immediate cast operands
        if (cond->kind == ND_CAST && cond->lhs && cond->lhs->ty &&
            is_flonum(cond->lhs->ty) && cond->lhs->kind != ND_FNUM)
            warn_tok(cond->tok,
                     "static_assert condition is not an integer constant expression");
        long long val = 0;
        if (!eval_const_expr(cond, &val))
            error_tok(cond->tok, "static_assert condition must be a constant expression");
        char *msg = "static_assert failed";
        if (equalc(tok, ",")) {
            tok = tok->next;
            if (tok->kind == TK_STR) {
                msg = tok->str;
                tok = tok->next;
            } else {
                error_tok(tok, "expected string literal in static assertion");
            }
        }
        tok = skip(tok, ")");
        tok = skip(tok, ";");
        if (!val)
            error_tok(start, "%s", msg);
        *rest = tok;
        return new_node(ND_NULL, tok);
    }

    VarAttr attr = {};
    pending_cleanup_func = NULL;
    pending_constructor = false;
    pending_destructor = false;
    pending_asm_name = NULL;
    pending_alias_target = NULL;
    pending_section_name = NULL;
    if (pending_target_clones) {
        free(pending_target_clones);
        pending_target_clones = NULL;
    }
    pending_target_clones_n = 0;
    pending_target_attr = NULL;
    Type *base = declspec(&tok, tok, &attr);
    char *type_level_cleanup = pending_cleanup_func;
    Node head = {};
    Node *cur = &head;

    if (equalc(tok, ";")) {
        pending_cleanup_func = NULL;
        *rest = tok->next;
        return new_node(ND_NULL, tok);
    }

    while (!equalc(tok, ";")) {
        char *name = NULL;
        int decl_align = 0;
        pending_cleanup_func = NULL;
        Type *ty = declarator(&tok, tok, copy_type(base), &name, &attr);
        // C11 6.7.4p2: _Noreturn only on function declarations
        if (attr.is_noreturn_std && ty->kind != TY_FUNC)
            error_tok(tok, "'_Noreturn' on a non-function declaration");
        tok = read_type_attrs(tok, &decl_align, NULL);
        if (decl_align > 0 && ty->kind == TY_FUNC) {
            ty = copy_type(ty);
            ty->align = decl_align;
        }
        char *cleanup = pending_cleanup_func ? pending_cleanup_func : type_level_cleanup;
        pending_cleanup_func = NULL;
        if (!name)
            error_tok(tok, "expected variable name");

        if (ty->kind != TY_FUNC && name && equalc(tok, "(")) {
            // K&R-style (old-style) nested function definition, e.g.
            // `void r(a) { ... }` - declarator() already consumed `r`
            // (name is set) but type_suffix's own K&R detection saw an
            // identifier-only parameter list and bailed out, leaving
            // `ty` as the bare return type with `tok` still at the
            // un-consumed `(` (see type_suffix's "Detect old-style
            // (K&R) parameter lists" comment). This mirrors the
            // top-level K&R function-definition path in parse() (this
            // project's own), reusing parse_kr_param_list for the
            // shared name+type resolution logic.
            Token *ptok = tok->next; // skip "("
            KRParam *kr_params = parse_kr_param_list(&tok, ptok);
            Type *fty = func_type(ty);
            fty->is_oldstyle = true;
            Type param_head = {0};
            Type *pcur = &param_head;
            for (KRParam *krp = kr_params; krp; krp = krp->next) {
                Type *pt = arena_alloc(sizeof(Type));
                *pt = krp->ty ? *krp->ty : *ty_int;
                pt->param_next = NULL;
                pcur = pcur->param_next = pt;
            }
            fty->param_types = param_head.param_next;

            LVar *fn_sym = find_global_name(name);
            if (!fn_sym) {
                fn_sym = new_var(name, pointer_to(fty), false);
                fn_sym->is_extern = true;
                fn_sym->is_function = true;
            }
            LVar *lvar = arena_alloc(sizeof(LVar));
            lvar->name = name;
            lvar->ty = pointer_to(fty);
            lvar->is_local = false;
            lvar->is_extern = true;
            lvar->is_function = true;
            lvar->next = locals;
            locals = lvar;
            if (current_block_depth == 1)
                current_fn_scope_locals = locals;

            char *mangled = format(".L.nest.%s.%s.%d", parser_current_fn ? parser_current_fn : "",
                                   name, nested_fn_counter++);
            lvar->asm_name = mangled;
            lvar->is_nested_fn = true;
            tok = parse_nested_function_def(&tok, tok, fty, name, mangled, kr_params);
            *rest = tok;
            return head.next;
        }

        // A function-type TYPEDEF (`typedef int functype(int);`) must NOT
        // take the nested-function-declaration path below: that path
        // registers `name` as a local function symbol/prototype and never
        // calls add_typedef, so a later `(functype *) fn` cast silently
        // fails is_typename() and misparses as "expected an expression"
        // (found via jimsh0.c's `typedef int (qsort_comparator)(const
        // void*, const void*);` inside ListSort()).
        if (ty->kind == TY_FUNC && !attr.is_typedef) {
            Type *fty = ty;
            LVar *fn_sym = find_global_name(name);
            if (!fn_sym) {
                fn_sym = new_var(name, pointer_to(fty), false);
                fn_sym->is_extern = true;
                fn_sym->is_function = true;
                fn_sym->is_weak = attr.is_weak;
                fn_sym->has_visibility = attr.has_visibility;
                fn_sym->visibility = attr.visibility;
                fn_sym->is_reproducible = attr.is_reproducible;
                fn_sym->is_unsequenced = attr.is_unsequenced;
            } else {
                // A block-scope prototype redeclaring a file-scope function
                // must still be type-compatible with it (C11 6.2.7p2) --
                // gnulib's ioctl POSIX-signature configure probe declares
                // `int ioctl (int, int, ...);` inside main() to see whether
                // it conflicts with glibc's unsigned-long prototype; rcc
                // silently accepted it, so gnutls' configure set
                // REPLACE_IOCTL=0 and the generated sys/ioctl.h took the
                // SYS branch, redeclaring ioctl with `int request` and
                // failing the build. gcc errors here, REPLACE_IOCTL=1, and
                // the rpl_ioctl path is taken instead.
                if (!fn_sym->is_synthetic_prelude && fn_sym->ty && fn_sym->ty->base &&
                    !fty->is_oldstyle &&
                    func_decls_conflict(fn_sym->ty->base, fty))
                    error_tok(tok, "conflicting types for '%s'", name);
                // Preserve alignment from prior declaration
                if (fn_sym->ty && fn_sym->ty->base && fn_sym->ty->base->align > fty->align)
                    fty->align = fn_sym->ty->base->align;
                if (attr.is_weak)
                    fn_sym->is_weak = true;
            }
            // Create local entry so this function declaration shadows any local variable
            LVar *lvar = arena_alloc(sizeof(LVar));
            lvar->name = name;
            lvar->ty = pointer_to(fty);
            lvar->is_local = false;
            lvar->is_extern = true;
            lvar->is_function = true;
            lvar->is_weak = attr.is_weak;
            lvar->has_visibility = attr.has_visibility;
            lvar->visibility = attr.visibility;
            if (pending_asm_name)
                lvar->asm_name = pending_asm_name;
            lvar->next = locals;
            locals = lvar;
            if (current_block_depth == 1)
                current_fn_scope_locals = locals;
            // Consume GCC function specifiers like __cond_acquires(true, lock)
            while (tok->kind == TK_IDENT && tok->next && equalc(tok->next, "(")) {
                tok = tok->next;
                tok = skip(tok, "(");
                int pdepth = 1;
                while (pdepth > 0 && tok->kind != TK_EOF) {
                    if (equalc(tok, "(")) pdepth++;
                    else if (equalc(tok, ")"))
                        pdepth--;
                    tok = tok->next;
                }
                if (equalc(tok, ","))
                    tok = tok->next;
            }
            // GNU nested function: `int foo(params) { body }` appearing as
            // a statement inside another function's body (this branch is
            // block-scope-only; the file-scope function-definition path in
            // parse() has its own '{' handling and never reaches here).
            if (equalc(tok, "{")) {
                for (Type *pt = fty->param_types; pt; pt = pt->param_next)
                    if (pt->vla_len_expr || (pt->kind == TY_PTR && pt->base && pt->base->vla_len_expr))
                        error_tok(tok, "nested function '%s' with a VLA-typed "
                                       "parameter is not yet supported",
                                  name);
                char *mangled = format(".L.nest.%s.%s.%d", parser_current_fn ? parser_current_fn : "",
                                       name, nested_fn_counter++);
                lvar->asm_name = mangled;
                lvar->is_nested_fn = true;
                tok = parse_nested_function_def(&tok, tok, fty, name, mangled, NULL);
                *rest = tok;
                return head.next;
            }
            if (!equalc(tok, ","))
                break;
            tok = tok->next;
            continue;
        }

        // -W: warn when a local declaration shadows a typedef name
        if (opt_W && !attr.is_typedef && name && typedef_find_name(name))
            warn_tok(tok, "declaration of '%s' shadows a global declaration", name);

        if (attr.is_typedef) {
            add_typedef(name, ty);
        } else if (attr.is_register && attr.has_alignas) {
            error_tok(tok, "alignment specified for register variable");
        } else if (attr.is_static) {
            // C11 6.7.4p3: a non-static inline function may not define a
            // modifiable object with static storage duration.
            if (current_fn_is_inline && !(ty->qual & QUAL_CONST))
                error_tok(tok, "'%s' is static but declared in inline function '%s'",
                          name, parser_current_fn ? parser_current_fn : "?");
            // Static local variable: create global storage with unique name
            char *asm_label = format(".Lstatic.%d", static_local_counter++);
            if (equalc(tok, "="))
                ty = infer_array_type(ty, tok->next);
            if (ty->kind == TY_VLA)
                error_tok(tok, "storage size of '%s' is not constant", name);
            // Global entry for storage
            LVar *gvar = arena_alloc(sizeof(LVar));
            gvar->name = asm_label;
            gvar->ty = ty;
            gvar->is_local = false;
            gvar->decl_fn_name = parser_current_fn;
            gvar->is_static = true;
            gvar->next = globals;
            globals = gvar;
            // Register in the name hash table too: read_global_label_initializer()'s
            // chained "&identifier[N][M].member..." address-constant continuation
            // (used when ANOTHER static's initializer references this static's
            // address, e.g. `static char buf[8][8]; static char *p = buf[0];` or
            // ecpg's `static struct variable v[2] = { {names[0], ...}, ... }`)
            // looks the label back up via find_global_name(), which is a pure
            // hash-table lookup with no linked-list fallback -- without this,
            // that lookup always misses for a local static (never registered
            // here before), silently skipping the "[N]"/".member" chain and
            // leaving those tokens unconsumed for the caller to choke on
            // ("expected specific operator"). Plain identifier resolution
            // elsewhere (via the `locals` list `lvar` entry below) was
            // unaffected, so ordinary local-static usage never hit this.
            global_htab_add(gvar);
            // Local entry for name lookup
            LVar *lvar = arena_alloc(sizeof(LVar));
            lvar->name = name;
            lvar->asm_name = pending_asm_name ? pending_asm_name : asm_label;
            lvar->ty = ty;
            lvar->is_local = false;
            lvar->is_static = true;
            lvar->next = locals;
            locals = lvar;
            if (equalc(tok, "=")) {
                tok = tok->next;
                global_initializer(&tok, tok, gvar);
                lvar->ty = gvar->ty;
            }
        } else if (attr.is_extern) {
            // Block-scope extern declaration: refers to global storage
            LVar *gvar = find_global_name(name);
            // C11 6.2.2: thread-local must agree across declarations
            if (gvar && !gvar->is_function && gvar->is_tls != attr.is_tls)
                error_tok(tok, "'%s' redeclared with different thread-local storage",
                          name);
            if (!gvar) {
                gvar = new_var(name, ty, false);
                gvar->is_extern = true;
                // A block-scope `extern` only *references* file-scope
                // storage - it does not own it the way a block-scope
                // `static` does. new_var() unconditionally stamps
                // decl_fn_name = parser_current_fn for any non-local var
                // created while inside a function body, so without this
                // reset, opt.c's eliminate_unused_static_inline() would
                // treat this global as if it belonged to (and must be
                // dropped alongside) the enclosing function whenever that
                // function is itself unused dead code - even though a
                // later file-scope definition of the same name reuses
                // this exact LVar and legitimately needs to survive.
                gvar->decl_fn_name = NULL;
            } else if (gvar->ty->kind == TY_ARRAY && ty->kind == TY_ARRAY && ty->size > 0 && gvar->ty->size == 0) {
                gvar->ty = ty;
            }
            if (pending_asm_name)
                gvar->asm_name = pending_asm_name;
            // Create local entry that references the global
            LVar *lvar = arena_alloc(sizeof(LVar));
            lvar->name = name;
            lvar->ty = gvar->ty;
            lvar->is_local = false;
            lvar->is_extern = true;
            if (pending_asm_name)
                lvar->asm_name = pending_asm_name;
            lvar->next = locals;
            locals = lvar;
            if (current_block_depth == 1)
                current_fn_scope_locals = locals;
        } else if (attr.is_constexpr && !attr.is_auto_type &&
                   (ty->kind == TY_STRUCT || ty->kind == TY_UNION || ty->kind == TY_ARRAY)) {
            // Aggregate constexpr locals (struct/union/array): no compile-time
            // scalar folding here (member/element const-eval isn't needed
            // unless the object is later used in a static_assert, which the
            // scalar-only fast path above still handles for non-aggregates).
            // Reuse the general local-init machinery (local_init_one) that
            // already handles brace lists, nested aggregates, and copy-init
            // from another variable.
            if (equalc(tok, "="))
                ty = infer_array_type(ty, tok->next);
            LVar *var = new_var(name, ty, true);
            var->is_constexpr = true;
            if (pending_asm_name)
                var->asm_name = pending_asm_name;
            var->ty = qualify_type_copy(ty, QUAL_CONST);
            if (current_block_depth == 1)
                current_fn_scope_locals = locals;
            if (!equalc(tok, "="))
                error_tok(tok, "constexpr variable must be initialized");
            Token *start = tok;
            tok = tok->next;
            Token *saved_after_eq = tok;
            Node *lhs = new_var_node(var, start);
            if (var->ty->size > 0) {
                Node *zinit = new_node(ND_ZERO_INIT, start);
                zinit->lhs = new_var_node(var, start);
                cur = cur->next = new_unary(ND_EXPR_STMT, zinit, start);
            }
            bool saved_ici1 = in_constexpr_init;
            in_constexpr_init = true;
            tok = local_init_one(tok, lhs, var->ty, &cur);
            in_constexpr_init = saved_ici1;
            // Also populate init_data for compile-time member access.
            // Re-parse the initializer with global_initializer which fills init_data.
            Token *post_init = tok;
            tok = saved_after_eq;
            // global_initializer can't find local variables. If the initializer
            // is a bare identifier or { identifier }, look up locals too.
            LVar *src_var = NULL;
            {
                Token *chk = tok;
                if (equalc(chk, "{")) chk = chk->next;
                if (chk && chk->kind == TK_IDENT) {
                    for (LVar *lv = locals; lv; lv = lv->next) {
                        if (lv->name == chk->name) {
                            src_var = lv;
                            break;
                        }
                    }
                    if (src_var && src_var->is_constexpr && src_var->init_data) {
                        var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
                        var->init_size = var->ty->size;
                        memcpy(var->init_data, src_var->init_data,
                               src_var->ty->size < var->ty->size ? src_var->ty->size : var->ty->size);
                        var->has_init = true;
                    } else {
                        src_var = NULL;
                    }
                }
            }
            if (!src_var) {
                // General case: use global_initializer for brace-enclosed init with literals.
                // Re-parses the same tokens local_init_one already emitted
                // runtime code for above (already warned there if needed);
                // speculative-gate so this second pass stays silent.
                bool saved_ici2 = in_constexpr_init;
                bool saved_spec = in_speculative_const_fold;
                in_constexpr_init = true;
                in_speculative_const_fold = true;
                global_initializer(&tok, tok, var);
                in_constexpr_init = saved_ici2;
                in_speculative_const_fold = saved_spec;
            }
            tok = post_init;
        } else if (attr.is_constexpr) {
            if (attr.is_auto_type) {
                // constexpr auto: infer type from initializer, then apply constexpr constraints
                if (!equalc(tok, "="))
                    error_tok(tok, "constexpr auto requires an initializer");
                Token *start = tok;
                tok = tok->next;
                Node *init_expr = NULL;
                // Handle compound literal (type){init}: extract inner value
                if (find_compound_literal_start(start->next)) {
                    tok = start->next;
                    int pd = 0;
                    while (pd > 0 || !equalc(tok, ")")) {
                        if (equalc(tok, "(")) pd++;
                        else if (equalc(tok, ")"))
                            pd--;
                        if (pd > 0) tok = tok->next;
                    }
                    if (tok) tok = tok->next;
                    tok = skip(tok, "{");
                    init_expr = conditional(&tok, tok);
                    tok = skip(tok, "}");
                } else {
                    init_expr = expr(&tok, tok);
                }
                check_type(init_expr);
                if (!init_expr->ty)
                    error_tok(start, "cannot infer type from constexpr auto initializer");
                Type *inferred = init_expr->ty;
                LVar *var = new_var(name, inferred, true);
                var->is_constexpr = true;
                var->ty = qualify_type_copy(inferred, QUAL_CONST);
                if (pending_asm_name)
                    var->asm_name = pending_asm_name;
                // For aggregate types (struct/union/array) initialized from another
                // constexpr variable, copy init_data for compile-time member access
                if ((inferred->kind == TY_STRUCT || inferred->kind == TY_UNION || inferred->kind == TY_ARRAY) &&
                    init_expr->kind == ND_LVAR && init_expr->var && init_expr->var->is_constexpr &&
                    init_expr->var->init_data) {
                    var->init_data = arena_alloc(inferred->size ? inferred->size : 1);
                    var->init_size = inferred->size;
                    memcpy(var->init_data, init_expr->var->init_data, inferred->size);
                    var->has_init = true;
                }
                long long val = 0;
                // For compound literal expressions, extract scalar value from the AST
                // compound literal of scalar type creates ND_COMMA(ND_ASSIGN(ND_LVAR, NUM), ND_LVAR)
                // eval_const_expr fails on the ND_LVAR RHS, so extract directly from the assignment
                // Walk through nested compound literal wrappers ND_COMMA(ND_ASSIGN(_, value), ND_LVAR)
                Node *value_node = init_expr;
                while (value_node->kind == ND_COMMA && value_node->lhs->kind == ND_ASSIGN &&
                       value_node->rhs->kind == ND_LVAR) {
                    value_node = value_node->lhs->rhs;
                }
                if (eval_const_expr(value_node, &val)) {
                    var->has_init = true;
                    var->init_val = (int64_t)val;
                }
                if (eval_const_expr(init_expr, &val)) {
                    var->has_init = true;
                    var->init_val = (int64_t)val;
                }
                Node *lhs = new_var_node(var, start);
                cur = cur->next = new_unary(ND_EXPR_STMT, new_binary(ND_ASSIGN, lhs, init_expr, start), start);
            } else {
                LVar *var = new_var(name, ty, true);
                var->is_constexpr = true;
                if (pending_asm_name)
                    var->asm_name = pending_asm_name;
                // constexpr implies const
                var->ty = qualify_type_copy(ty, QUAL_CONST);
                if (!equalc(tok, "="))
                    error_tok(tok, "constexpr variable must be initialized");
                Token *start = tok;
                tok = tok->next;
                Node *init_expr;
                // Handle brace-enclosed scalar initializer: { value }
                if (equalc(tok, "{")) {
                    tok = tok->next;
                    init_expr = expr(&tok, tok);
                    tok = skip(tok, "}");
                } else {
                    init_expr = expr(&tok, tok);
                }
                long long val = 0;
                if (!eval_const_expr(init_expr, &val))
                    error_tok(start, "constexpr variable must have a constant initializer");
                var->has_init = true;
                var->init_val = (int64_t)val;
                // Emit runtime initialization from the folded constant value
                Node *lhs = new_var_node(var, start);
                Node *rhs = new_num(val, start);
                Node *assign = new_binary(ND_ASSIGN, lhs, rhs, start);
                check_type(assign);
                cur = cur->next = new_unary(ND_EXPR_STMT, assign, start);
            }
        } else if (attr.is_auto_type) {
            if (!equalc(tok, "="))
                error_tok(tok, "__auto_type requires an initializer");
            for (LVar *lv = locals; lv; lv = lv->next)
                if (lv->name == name && lv->is_extern)
                    error_tok(tok, "underspecified declaration of '%s', already declared", name);
            Token *start = tok;
            tok = tok->next;
            Node *init_expr = expr(&tok, tok);
            check_type(init_expr);
            if (!init_expr->ty)
                error_tok(start, "__auto_type cannot infer type from initializer");
            Type *inferred = init_expr->ty;
            LVar *var = new_var(name, inferred, true);
            if (pending_asm_name)
                var->asm_name = pending_asm_name;
            Node *lhs = new_var_node(var, start);
            Node *assign = new_binary(ND_ASSIGN, lhs, init_expr, start);
            check_type(assign);
            cur = cur->next = new_unary(ND_EXPR_STMT, assign, start);
        } else {
            if (equalc(tok, "=")) {
                ty = infer_array_type(ty, tok->next);
            }
            // Freeze pointer-to-VLA dimensions (e.g. `typeof (int (*)[++i]) p`)
            // and, for a VLA-kind declarator itself (e.g. `int (*p[f(2)])[f(3)]`,
            // an array of pointers to a VLA-array), every dimension expression
            // reachable through the pointer/VLA chain - so each dim's side
            // effect runs exactly once at the declaration (C11 6.7.6.2p5)
            // rather than being re-evaluated later by e.g. `sizeof p` /
            // `sizeof *p` reusing the same unevaluated expression node.
            Node *vla_pre = NULL;
            if (parser_current_fn && (ty->kind == TY_PTR || ty->kind == TY_VLA))
                ty = vla_freeze_dims(ty, &vla_pre, tok);
            // C11 6.7p7 (via 6.2.5p28's "incomplete type" definition):
            // an object with automatic storage duration must have a
            // complete type -- there is no way to reserve stack space for
            // an opaque forward-declared struct/union whose size is
            // unknown. GCC: "storage size of 'p' isn't known". Found via
            // Tcl's own `./configure` LFS probe (`struct dirent64 p;`,
            // no `#include`d definition beyond the opaque glibc forward
            // declaration without _LARGEFILE64_SOURCE/_GNU_SOURCE):
            // rcc silently accepted this where real gcc rejects it,
            // flipping the HAVE_STRUCT_DIRENT64 autoconf result and
            // making tclUnixPort.h select a dirent64/d_name path real
            // gcc's own Tcl build never takes on this system.
            if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && !ty->has_body)
                error_tok(tok, "storage size of '%s' isn't known", name);
            LVar *var = new_var(name, ty, true);
            // Flush queued typeof(VM expr) evaluations and struct-size
            // captures ahead of this declarator's own dim freezes.
            if (pending_vla_struct_capture) {
                cur = cur->next = pending_vla_struct_capture;
                while (cur->next)
                    cur = cur->next;
                pending_vla_struct_capture = NULL;
            }
            if (vla_pre)
                cur = cur->next = new_unary(ND_EXPR_STMT, vla_pre, tok);
            if (pending_asm_name)
                var->asm_name = pending_asm_name;
            var->cleanup_func = cleanup ? cleanup : ty->cleanup_func;
            if (attr.is_tls)
                error_tok(tok, "'__thread'/'_Thread_local' at block scope requires 'static' or 'extern'");
            var->is_tls = attr.is_tls;
            // VLA: compute size and allocate stack space
            if (ty->kind == TY_VLA) {
                Node *vla_node = new_node(ND_ALLOCA, tok);
                // C23: `T vla[n] = {}` empty-initializes (zero-fills) the VLA.
                // VLAs admit no other initializer, so any `=` here is the empty
                // one; allocate-and-zero in a single ALLOCA_ZINIT.
                if (equalc(tok, "="))
                    vla_node->kind = ND_ALLOCA_ZINIT;
                vla_node->lhs = vla_alloc_size(ty, tok);
                vla_node->var = var;
                cur = cur->next = new_unary(ND_EXPR_STMT, vla_node, tok);
                fn_uses_vla = true;
            } else if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->vla_len_expr) {
                // VLA-containing struct: emit the pending size capture first (to set the cap
                // lvar that ty->vla_len_expr references), then allocate the VLA data area.
                if (pending_vla_struct_capture) {
                    cur = cur->next = pending_vla_struct_capture;
                    while (cur->next)
                        cur = cur->next;
                    pending_vla_struct_capture = NULL;
                }
                Node *vla_node = new_node(ND_ALLOCA, tok);
                vla_node->lhs = ty->vla_len_expr;
                vla_node->var = var;
                cur = cur->next = new_unary(ND_EXPR_STMT, vla_node, tok);
                fn_uses_vla = true;
            }

            if (current_block_depth == 1)
                current_fn_scope_locals = locals;
            if (equalc(tok, "=")) {
                Token *start = tok;
                tok = tok->next;
                Node *lhs = new_var_node(var, start);
                // Zero-initialize aggregate locals before specific initializers
                // so unspecified elements are 0 as required by C.
                if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION ||
                     var->ty->kind == TY_ARRAY) &&
                    var->ty->size > 0) {
                    Node *zinit = new_node(ND_ZERO_INIT, start);
                    zinit->lhs = new_var_node(var, start);
                    cur = cur->next = new_unary(ND_EXPR_STMT, zinit, start);
                }
                if (var->ty->kind == TY_VLA) {
                    // The ALLOCA_ZINIT above already zeroed the VLA; just
                    // consume the (necessarily empty) `{}` initializer.
                    tok = skip_initializer(tok);
                } else {
                    tok = local_init_one(tok, lhs, var->ty, &cur);
                }
            }
        }

        if (!equalc(tok, ","))
            break;
        tok = tok->next;
    }

    pending_asm_name = NULL;
    pending_alias_target = NULL;
    pending_section_name = NULL;
    *rest = skip(tok, ";");
    return head.next ? head.next : new_node(ND_NULL, tok);
}

// Statement-level error recovery (GH #34): checkpoints refreshed at every
// statement so all statements of a block are diagnosed, not just the first.
// Shared across block nesting; the innermost active block re-arms them at
// each statement, so they always describe the current statement.
static TypedefLog *stmt_rec_td;
static TagLog *stmt_rec_tag;
static EnumLog *stmt_rec_enum;
static Token *stmt_iter_tok;

// Skip tokens to the next statement boundary inside a block: past the next
// ';' at the current brace depth, or to (not past) the '}' closing this
// block so the statement loop terminates normally.
static Token *sync_stmt(Token *tok) {
    int depth = 0;
    while (tok->kind != TK_EOF) {
        if (equalc(tok, "{")) {
            depth++;
        } else if (equalc(tok, "}")) {
            if (depth == 0) {
                // Don't return at a '}' that is part of a compound literal
                // or initializer (followed by '.' , ';' or similar).
                // Only return if this is the block's own closing '}' —
                // no ';' directly after it (a compound literal always has ';'
                // after its closing '}').
                if (!equalc(tok->next, ";") && !equalc(tok->next, ".") &&
                    !equalc(tok->next, ")") && !equalc(tok->next, "]"))
                    return tok;
                // Compound literal } followed by something — keep going
            } else {
                depth--;
            }
        } else if (equalc(tok, ";") && depth == 0) {
            return tok->next;
        }
        tok = tok->next;
    }
    return tok;
}

// Skip to the closing '}' of the statement-expression block the error token
// sits in, returning that '}' (unconsumed) so the block's statement loop
// exits normally and scopes restore. Statement-expressions are parsed as
// nested compound statements with their own statement-level recovery point;
// without this the recovery resumed at the next INNER statement (sync_stmt
// stops at the block's own ';'s), so an error inside a macro like
// atomic_load_explicit's ({ ... }) cascaded into the macro's remaining
// statements (re-reporting the builtin failure and an undeclared temp),
// while tinycc aborts after the first error (125_atomic_misc).
static Token *sync_stmt_expr_block(Token *tok) {
    int brace_depth = 0;
    int stmt_expr_parens = 0;
    while (tok->kind != TK_EOF) {
        if (equalc(tok, "(") && tok->next && equalc(tok->next, "{")) {
            stmt_expr_parens++;
            tok = tok->next; // skip '('; the '{' is counted below
        } else if (equalc(tok, "{")) {
            brace_depth++;
        } else if (equalc(tok, "}")) {
            if (brace_depth == 0) {
                if (stmt_expr_parens > 0 && tok->next && equalc(tok->next, ")")) {
                    // closing "})" of a nested statement-expression
                    stmt_expr_parens--;
                    tok = tok->next; // skip ')'
                } else {
                    return tok; // this block's closing '}'
                }
            } else {
                brace_depth--;
            }
        }
        tok = tok->next;
    }
    return tok;
}

static Node *compound_stmt_ex(Token **rest, Token *tok, LVar **out_locals,
                              bool is_stmt_expr) {
    LVar *saved_locals = locals;
    Typedef *saved_typedefs = typedefs;
    TagScope *saved_tags = tags;
    EnumConst *saved_enum_consts = enum_consts;
    TypedefLog *saved_typedef_log = typedef_scope_checkpoint();
    TagLog *saved_tag_log = tag_scope_checkpoint();
    EnumLog *saved_enum_log = enum_scope_checkpoint();
    int saved_block_depth = current_block_depth;
    int saved_fenv_access = fenv_access;

    Node head = {};
    Node *volatile cur = &head;
    tok = skip(tok, "{");
    current_block_depth++;

    // Statement-level recovery point: error_tok() longjmps here from inside
    // a statement (see error_finish). The parent block's recovery state is
    // saved on this frame and restored on exit, so nesting unwinds correctly.
    jmp_buf saved_stmt_jmp;
    bool saved_stmt_active = stmt_recovery_active;
    if (saved_stmt_active)
        memcpy(saved_stmt_jmp, stmt_recovery_jmp, sizeof(jmp_buf));
    int rec_depth = current_block_depth;
    if (setjmp(stmt_recovery_jmp)) {
        current_block_depth = rec_depth;
        typedef_scope_restore(stmt_rec_td);
        tag_scope_restore(stmt_rec_tag);
        enum_scope_restore(stmt_rec_enum);
        pending_vla_struct_capture = NULL;
        pending_cleanup_func = NULL;
        if (is_stmt_expr)
            // Skip the rest of the ({ ... }) block; the enclosing statement
            // finishes normally, no cascading errors (see sync_stmt_expr_block).
            tok = sync_stmt_expr_block(error_recovery_tok);
        else
            tok = sync_stmt(error_recovery_tok);
        if (tok == stmt_iter_tok && tok->kind != TK_EOF)
            tok = sync_stmt(tok->next); // no forward progress: force a skip

        if (tok->kind == TK_EOF) {
            // Ran off the block: unwind to the top-level recovery point
            // (all statement frames die, so deactivate this level).
            stmt_recovery_active = false;
            longjmp(error_recovery_jmp, 1);
        }
    }
    stmt_recovery_active = true;

    while (!equalc(tok, "}")) {
        // Checkpoint per statement for error recovery.
        stmt_iter_tok = tok;
        stmt_rec_td = typedef_scope_checkpoint();
        stmt_rec_tag = tag_scope_checkpoint();
        stmt_rec_enum = enum_scope_checkpoint();

        // Handle # pragma pack(N) emitted by the preprocessor
        if (equalc(tok, "#") && equalc(tok->next, "pragma") &&
            equalc(tok->next->next, "pack")) {
            tok = tok->next->next->next;
            if (equalc(tok, "(")) {
                tok = tok->next;
                if (tok->kind == TK_NUM)
                    pack_align = tok->val;
                else
                    pack_align = 0;
                tok = tok->next;
                if (equalc(tok, ")"))
                    tok = tok->next;
            }
            continue;
        }

        // Handle # pragma fenv(N) emitted by the preprocessor
        if (equalc(tok, "#") && equalc(tok->next, "pragma") &&
            equalc(tok->next->next, "fenv")) {
            tok = tok->next->next->next;
            if (equalc(tok, "(")) {
                tok = tok->next;
                if (tok->kind == TK_NUM)
                    fenv_access = tok->val;
                else
                    fenv_access = false;
                tok = tok->next;
                if (equalc(tok, ")"))
                    tok = tok->next;
            }
            continue;
        }
        // _Pragma("string") — C99 pragma operator, not a statement (see
        // stmt()'s identical no-op below). Must vanish here too, before
        // reaching the body list: otherwise a trailing _Pragma inside a
        // GNU statement-expression `({ ...; expr; _Pragma(...); })` (the
        // "GCC diagnostic push/ignored/pop" idiom around a pointer cast,
        // e.g. grep's skip_easy_bytes CAST_ALIGNED macro) becomes the
        // last node in body, isn't an ND_EXPR_STMT, and silently drops
        // the intended result value.
        if (equalc(tok, "_Pragma")) {
            tok = tok->next;
            tok = skip(tok, "(");
            if (tok->kind == TK_STR)
                tok = tok->next;
            tok = skip(tok, ")");
            continue;
        }
        // Standalone __attribute__((...)) at statement level (e.g. __fallthrough__)
        if ((equalc(tok, "__attribute__") || equalc(tok, "__attribute"))) {
            Token *after = peek_past_attr(tok);
            if (after && equalc(after, ";")) {
                cur->next = stmt(&tok, tok);
                while (cur->next)
                    cur = cur->next;
                continue;
            }
        }
        // __label__ declaration for GNU C local label variables
        if (equalc(tok, "__label__")) {
            tok = tok->next;
            while (tok->kind == TK_IDENT) {
                record_label_scope(tok->name, locals);
                tok = tok->next;
                if (equalc(tok, ",")) tok = tok->next;
            }
            tok = skip(tok, ";");
            continue;
        }
        // C23 static_assert / C11 _Static_assert at block scope
        // C23 static_assert / C11 _Static_assert at block scope
        if (equalc(tok, "static_assert") || equalc(tok, "_Static_assert")) {
            cur->next = declaration(&tok, tok);
            while (cur->next)
                cur = cur->next;
            continue;
        }
        // C23 [[attribute]] at statement level
        if (equalc(tok, "[") && equalc(tok->next, "[") && tok->ptr + tok->len == tok->next->ptr) {
            if (opt_pedantic && opt_std_version && strcmp(opt_std_version, "202311L") < 0) {
                warn_tok(tok, "[[attributes]] before C23 are not supported");
                int bd = 1;
                Token *tt = tok->next->next;
                while (tt && tt->kind != TK_EOF && bd > 0) {
                    if (equalc(tt, "[") && equalc(tt->next, "[") && tt->ptr + tt->len == tt->next->ptr) bd++;
                    else if (equalc(tt, "]") && equalc(tt->next, "]")) {
                        bd--;
                        tt = tt->next;
                    }
                    tt = tt->next;
                }
                tok = tt;
                continue;
            }
            Token *t = tok->next->next;
            bool empty_attr = equalc(t, "]") && t->next && equalc(t->next, "]") &&
                t->ptr + t->len == t->next->ptr;
            Token *after_attr = empty_attr ? t->next->next : NULL;
            int depth = 1;
            while (depth > 0 && t->kind != TK_EOF) {
                if (equalc(t, "[") && equalc(t->next, "[") && t->ptr + t->len == t->next->ptr) depth++;
                else if (equalc(t, "]") && equalc(t->next, "]")) {
                    depth--;
                    t = t->next;
                }
                t = t->next;
            }
            // Empty [[]] before struct/union/enum is only invalid in C23+,
            // and only when the declaration truly has no declarator.
            if (empty_attr && after_attr && opt_std_version &&
                strcmp(opt_std_version, "202311L") >= 0 &&
                (equalc(after_attr, "struct") || equalc(after_attr, "union") ||
                 equalc(after_attr, "enum")) &&
                is_empty_tag_decl(after_attr))
                error_tok(after_attr, "empty declaration");
            // Attributes cannot prefix a static_assert-declaration
            if (opt_std_version && strcmp(opt_std_version, "202311L") >= 0 &&
                (equalc(t, "static_assert") || equalc(t, "_Static_assert")))
                error_tok(t, "expected declaration specifiers before static_assert");
            tok = t;
            continue;
        }
        // C23 __auto_type declaration
        if (equalc(tok, "__auto_type")) {
            cur->next = declaration(&tok, tok);
            while (cur->next)
                cur = cur->next;
            continue;
        }
        if (is_typename(tok)) {
            // A typedef name followed by ':' is a label, not a declaration.
            if (find_typedef(tok) && equalc(tok->next, ":")) {
                cur = cur->next = stmt(&tok, tok);
                continue;
            }
            // A typedef name shadowed by a local variable is an expression,
            // not a declaration (e.g. `unsigned char s; … s = expr;` when
            // a typedef `s` is also in scope).
            if (find_typedef(tok) && find_var(tok)) {
                cur = cur->next = stmt(&tok, tok);
                continue;
            }
            cur->next = declaration(&tok, tok);
            while (cur->next)
                cur = cur->next;
            // Emit VLA-struct size capture before any subsequent n++ can change it
            if (pending_vla_struct_capture) {
                cur = cur->next = pending_vla_struct_capture;
                while (cur->next)
                    cur = cur->next;
                pending_vla_struct_capture = NULL;
            }
            continue;
        }
        {
            // A bare statement (expression statement, if/while/etc.) can
            // also queue a VM-typeof side-effect evaluation while parsing
            // (e.g. `(typeof(c++, p))0;` or a bare
            // `__builtin_va_arg(ap, typeof(out--, p));`) - the declaration()
            // path above already flushes pending_vla_struct_capture, but
            // this fallback previously dropped it silently. Splice the
            // queued captures in ahead of the statement, same ordering
            // convention as the declaration path.
            Node *s = stmt(&tok, tok);
            if (pending_vla_struct_capture) {
                cur->next = pending_vla_struct_capture;
                while (cur->next)
                    cur = cur->next;
                pending_vla_struct_capture = NULL;
                cur = cur->next = s;
            } else {
                cur = cur->next = s;
            }
            while (cur->next)
                cur = cur->next;
        }
    }

    // Hand recovery back to the enclosing block (or none at file scope).
    stmt_recovery_active = saved_stmt_active;
    if (saved_stmt_active)
        memcpy(stmt_recovery_jmp, saved_stmt_jmp, sizeof(jmp_buf));

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    *rest = tok->next;

    if (out_locals)
        *out_locals = locals;
    else
        node->body = append_cleanup_range(node->body, locals, saved_locals, tok);
    current_block_depth = saved_block_depth;
    fenv_access = saved_fenv_access;
    locals = saved_locals;
    typedef_scope_restore(saved_typedef_log);
    tag_scope_restore(saved_tag_log);
    enum_scope_restore(saved_enum_log);
    typedefs = saved_typedefs;
    tags = saved_tags;
    enum_consts = saved_enum_consts;
    return node;
}

static Node *compound_stmt(Token **rest, Token *tok) {
    return compound_stmt_ex(rest, tok, NULL, false);
}

// Parse a K&R (old-style) parameter-name list and its following
// declaration-list: `tok` must be positioned right after the function's
// own `(` (i.e. at the first parameter name or `)`), with the caller
// already having confirmed this is K&R style (first token inside `(` is
// an identifier that isn't a typename). Returns the resolved name+type
// list (an unresolved name - declared nowhere between `)` and `{` -
// stays NULL, defaulting to `int` per K&R rules; the caller decides
// when to apply that default). `*rest` is left at the `{` opening the
// body. Deliberately returns names+types only, not LVars: a nested
// function's parameter LVars must be created *after* push_fn_ctx() has
// reset `locals`/`stack_offset` to the nested function's own fresh
// scope, so LVar creation is the caller's job.
static KRParam *parse_kr_param_list(Token **rest, Token *tok) {
    KRParam kr_head = {0};
    KRParam *kr_cur = &kr_head;
    while (!equalc(tok, ")")) {
        if (kr_cur != &kr_head)
            tok = skip(tok, ",");
        if (tok->kind != TK_IDENT)
            error_tok(tok, "expected parameter name");
        KRParam *krp = arena_alloc(sizeof(KRParam));
        krp->name = tok->name;
        krp->ty = NULL;
        tok = tok->next;
        kr_cur = kr_cur->next = krp;
    }
    tok = skip(tok, ")");
    tok = skip_attributes(tok);
    while (!equalc(tok, "{")) {
        // Malformed input (e.g. a misdetected K&R parameter list from
        // garbage that isn't really an old-style function definition at
        // all) can run this loop past the last real token without ever
        // reaching a '{' body. Without this check, declspec()/declarator()
        // called on the trailing TK_EOF sentinel silently fail to consume
        // it (EOF isn't a valid type-specifier token), so `tok` stays
        // pinned at EOF forever -- and declarator() unconditionally reads
        // `tok->next` a few lines in, which is NULL for the lexer's
        // genuine end-of-list EOF token, segfaulting several calls deeper
        // inside skip_attributes()/read_type_attrs(). A real parameter
        // declaration list running off the end of the file is a genuine
        // syntax error; diagnose it here instead of looping into EOF.
        if (tok->kind == TK_EOF)
            error_tok(tok, "expected '{' before end of input");
        VarAttr dattr = {};
        Type *dty = declspec(&tok, tok, &dattr);
        for (;;) {
            char *dname = NULL;
            Type *ddecl = declarator(&tok, tok, copy_type(dty), &dname, &dattr);
            if (dname) {
                // C11 6.7.6.3p7: array/VLA/function parameter types decay
                // to pointer — same adjustment declarator_params() already
                // applies for modern prototype-style parameters. Without
                // it, a K&R-style `int x[][4];` param stayed a genuine
                // (incomplete, size-0) array type, corrupting every
                // `x[i][j]` index computation (wrong element stride) and
                // the parameter's own register/stack ABI slot (arrays
                // aren't passed by value at all). Found via cc65's own
                // LCC-derived K&R test corpus (test/ref/array.c).
                if (ddecl->kind == TY_VLA) {
                    unsigned char pqual = ddecl->qual;
                    Node *vla_expr = ddecl->vla_len_expr; // size side effects must run per call (C11 6.7.6.3p7), like the prototype path does at fn entry
                    ddecl = pointer_to(ddecl->base);
                    ddecl->qual |= pqual;
                    for (KRParam *krp = kr_head.next; krp; krp = krp->next) {
                        if (krp->name == dname) {
                            krp->vla_len_expr = vla_expr;
                            break;
                        }
                    }
                } else if (ddecl->kind == TY_ARRAY) {
                    unsigned char pqual = ddecl->qual;
                    ddecl = pointer_to(ddecl->base);
                    ddecl->qual |= pqual;
                } else if (ddecl->kind == TY_FUNC) {
                    ddecl = pointer_to(ddecl);
                }
                for (KRParam *krp = kr_head.next; krp; krp = krp->next) {
                    if (krp->name == dname) {
                        krp->ty = ddecl;
                        break;
                    }
                }
            }
            if (!equalc(tok, ","))
                break;
            tok = tok->next;
        }
        tok = skip(tok, ";");
    }
    *rest = tok;
    return kr_head.next;
}

// GNU nested function: `int foo(params) { body }` appearing as a statement
// inside another function's body. `tok` points at the opening '{'; `fty`
// is the already-parsed TY_FUNC type from declarator() (its param_types
// list is reused verbatim via the same placeholder-LVar mechanism the
// top-level function-definition handler in parse() uses — see the
// `pt->vla_len_val` branch below, mirrored from there). `kr_params`, when
// non-NULL, is a resolved K&R name+type list from parse_kr_param_list
// (declaration()'s caller already detected the K&R bail-out and parsed
// it before calling here) - in that mode `fty`'s param_types are already
// built from `kr_params` too (for is_oldstyle/signature purposes), but
// param LVar creation uses `kr_params` directly instead of the
// modern-declarator placeholder-LVar mechanism below.
static Token *parse_nested_function_def(Token **rest, Token *tok, Type *fty,
                                        char *decl_name, char *mangled_name, KRParam *kr_params) {
    push_fn_ctx();

    LVar *params;
    if (kr_params) {
        LVar head = {0};
        LVar *cur = &head;
        for (KRParam *krp = kr_params; krp; krp = krp->next) {
            if (!krp->ty)
                krp->ty = ty_int;
            LVar *var = new_var(krp->name, krp->ty, true);
            cur = cur->param_next = var;
        }
        params = head.param_next;
    } else {
        LVar head = {0};
        LVar *cur = &head;
        int param_index = 0;
        for (Type *pt = fty->param_types; pt; pt = pt->param_next) {
            char *pname = pt->name ? pt->name : format("__param%d", param_index++);
            LVar *lvar;
            if (pt->vla_len_val) {
                lvar = (LVar *)pt->vla_len_val;
                if (pt->kind != TY_STRUCT && pt->kind != TY_UNION)
                    lvar->ty = pt;
                int sz = pt->size < 4 ? 4 : pt->size;
                int al = pt->align < 4 ? 4 : pt->align;
                stack_offset = align_to(stack_offset + sz, al);
                lvar->offset = stack_offset;
                lvar->next = locals;
                locals = lvar;
                lvar->param_next = NULL;
            } else {
                lvar = new_var(pname, pt, true);
            }
            cur = cur->param_next = lvar;
        }
        params = head.param_next;
    }
    current_fn_scope_locals = params;
    parser_current_fn = decl_name;

    if (fty->return_ty && (fty->return_ty->kind == TY_STRUCT || fty->return_ty->kind == TY_UNION || fty->return_ty->kind == TY_COMPLEX || (fty->return_ty->kind == TY_BITINT && fty->return_ty->size > 16))) {
        LVar *retbuf = new_var("__retbuf", pointer_to(fty->return_ty), true);
        retbuf->cleanup_func = NULL;
    }

    LVar *fn_locals = NULL;
    Node *body = compound_stmt_ex(&tok, tok, &fn_locals, false);

    Function *fn = arena_alloc(sizeof(Function));
    fn->name = decl_name;
    fn->asm_name = mangled_name;
    fn->ty = fty;
    fn->params = params;
    fn->locals = fn_locals;
    fn->body = body->body;
    fn->stack_size = align_to(stack_offset, 16);
    fn->is_variadic = fty->is_variadic;
    fn->is_nested = true;
    fn->is_static = true; // never externally visible; only reachable via its mangled name

    TLItem *item = arena_alloc(sizeof(TLItem));
    item->kind = TL_FUNC;
    item->fn = fn;
    tl_item_cur = tl_item_cur->next = item;

    pop_fn_ctx();
    *rest = tok;
    return tok;
}

static bool is_asm_keyword(Token *tok) {
    return tok->kw == ID_ASM || tok->kw == ID___ASM__ || tok->kw == ID___ASM;
}

#ifdef ARCH_ARM64
// Validate an ARM64 clobber register name.
static bool arm64_is_valid_clobber(const char *s) {
    if (!s || !*s) return false;
    if (strcmp(s, "memory") == 0 || strcmp(s, "cc") == 0) return true;
    // Integer registers: x0-x30, w0-w30, xzr, wzr, sp, wsp
    if ((s[0] == 'x' || s[0] == 'w') && s[1] >= '0' && s[1] <= '9') {
        int n = atoi(s + 1);
        return (n >= 0 && n <= 30) && (s[2] == '\0' || (n >= 10 && s[3] == '\0'));
    }
    if (strcmp(s, "xzr") == 0 || strcmp(s, "wzr") == 0 || strcmp(s, "sp") == 0 || strcmp(s, "wsp") == 0) return true;
    // FP/SIMD: d0-d31, s0-s31, q0-q31, v0-v31, h0-h31, b0-b31
    if ((s[0] == 'd' || s[0] == 's' || s[0] == 'q' || s[0] == 'v' || s[0] == 'h' || s[0] == 'b') &&
        s[1] >= '0' && s[1] <= '9') {
        int n = atoi(s + 1);
        return (n >= 0 && n <= 31);
    }
    return false;
}
#endif

static Node *parse_asm_stmt(Token **rest, Token *tok) {
    Node *node = new_node(ND_ASM, tok);
    tok = tok->next; // skip asm/__asm__/__asm

    // consume optional volatile/goto qualifiers
    while (equalc(tok, "volatile") || equalc(tok, "__volatile__") ||
           equalc(tok, "__volatile") || equalc(tok, "goto"))
        tok = tok->next;

    tok = skip(tok, "(");

    // Concatenate template string literals. Two passes: sum the exact
    // length needed first, then copy into an arena buffer sized exactly
    // -- a real inline-asm template can be arbitrarily long (e.g. xz's
    // hand-unrolled 8-level bittree range-decoder macro needs well over
    // 4096 bytes once its six/eight rc_asm_bittree() expansions are
    // concatenated). A fixed-size stack buffer here used to silently
    // drop every token that didn't fit once the running length crossed
    // that boundary (the length check gated the copy but `tok` still
    // advanced regardless), truncating the template -- for a template
    // whose overflow point fell early enough, effectively emptying it
    // and turning the whole inline-asm statement into a silent no-op.

    size_t tmpl_len = 0;
    for (Token *t = tok; t->kind == TK_STR; t = t->next)
        tmpl_len += (size_t)t->len;
    char *tbuf = arena_alloc(tmpl_len + 1);
    size_t tbuf_off = 0;
    while (tok->kind == TK_STR) {
        memcpy(tbuf + tbuf_off, tok->str, (size_t)tok->len);
        tbuf_off += (size_t)tok->len;
        tok = tok->next;
    }
    node->asm_template = str_intern(tbuf, (int)tbuf_off);

    if (equalc(tok, ")")) {
        *rest = skip(tok->next, ";");
        return node;
    }

    AsmOperand *ops = arena_alloc(sizeof(AsmOperand) * MAX_ASM_OPERANDS);
    for (int i = 0; i < MAX_ASM_OPERANDS; i++) ops[i].reg = -1;
    int nops = 0;

    // Parse output operands
    tok = skip(tok, ":");
    bool first = true;
    while (!equalc(tok, ":") && !equalc(tok, ")")) {
        if (!first) tok = skip(tok, ",");
        first = false;
        if (nops >= MAX_ASM_OPERANDS) error_tok(tok, "too many asm operands");
        AsmOperand *op = &ops[nops++];
        op->name[0] = '\0';
        if (equalc(tok, "[")) { // named operand [id]
            tok = tok->next;
            if (tok->kind == TK_IDENT) {
                int nlen = tok->len < (int)sizeof(op->name) - 1 ? tok->len : (int)sizeof(op->name) - 1;
                memcpy(op->name, tok->ptr, nlen);
                op->name[nlen] = '\0';
                tok = tok->next;
            }
            tok = skip(tok, "]");
        }
        if (tok->kind != TK_STR) error_tok(tok, "expected constraint string");
        int clen = tok->len < 15 ? tok->len : 15;
        memcpy(op->constraint, tok->str, clen);
        op->constraint[clen] = '\0';
        tok = tok->next;
        tok = skip(tok, "(");
        op->expr = expr(&tok, tok);
        check_type(op->expr);
        tok = skip(tok, ")");
        for (char *p = op->constraint; *p; p++) {
            if (*p == '=' || *p == '+') op->is_output = true;
            if (*p == '+') op->is_rw = true;
            if (*p == 'm') op->is_memory = true;
        }
    }
    int nout = nops;

    // Parse input operands
    if (!equalc(tok, ")")) {
        tok = skip(tok, ":");
        first = true;
        while (!equalc(tok, ":") && !equalc(tok, ")")) {
            if (!first) tok = skip(tok, ",");
            first = false;
            if (nops >= MAX_ASM_OPERANDS) error_tok(tok, "too many asm operands");
            AsmOperand *op = &ops[nops++];
            op->name[0] = '\0';
            if (equalc(tok, "[")) {
                tok = tok->next;
                if (tok->kind == TK_IDENT) {
                    int nlen = tok->len < (int)sizeof(op->name) - 1 ? tok->len : (int)sizeof(op->name) - 1;
                    memcpy(op->name, tok->ptr, nlen);
                    op->name[nlen] = '\0';
                    tok = tok->next;
                }
                tok = skip(tok, "]");
            }
            if (tok->kind != TK_STR) error_tok(tok, "expected constraint string");
            int clen = tok->len < 15 ? tok->len : 15;
            memcpy(op->constraint, tok->str, clen);
            op->constraint[clen] = '\0';
            tok = tok->next;
            tok = skip(tok, "(");
            op->expr = expr(&tok, tok);
            check_type(op->expr);
            tok = skip(tok, ")");
            for (char *p = op->constraint; *p; p++)
                if (*p == 'm') op->is_memory = true;
        }

        // Parse and validate clobbers
        if (!equalc(tok, ")")) {
            tok = skip(tok, ":");
            while (!equalc(tok, ":") && !equalc(tok, ")")) {
                if (tok->kind != TK_STR) error_tok(tok, "expected clobber string");
#ifdef ARCH_ARM64
                if (!arm64_is_valid_clobber(tok->str))
                    error_tok_simple(tok, "invalid clobber register '%s'", tok->str);
#endif
                tok = tok->next;
                if (equalc(tok, ",")) tok = tok->next;
            }

            // Parse goto labels
            if (!equalc(tok, ")")) {
                tok = skip(tok, ":");
                char **glabels = arena_alloc(sizeof(char *) * MAX_ASM_OPERANDS);
                int ngoto = 0;
                first = true;
                while (!equalc(tok, ")")) {
                    if (!first) tok = skip(tok, ",");
                    first = false;
                    if (tok->kind != TK_IDENT) error_tok(tok, "expected label name");
                    glabels[ngoto++] = tok->name;
                    tok = tok->next;
                }
                node->asm_goto_labels = glabels;
                node->asm_ngoto = ngoto;
            }
        }
    }

    node->asm_ops = ops;
    node->asm_nout = nout;
    node->asm_noperands = nops;

#ifdef ARCH_ARM64
    // Validate matching constraint references for extended inline asm
    for (int i = nout; i < nops; i++) {
        const char *c = ops[i].constraint;
        while (*c == '=' || *c == '+' || *c == '&' || *c == '%') c++;
        if (*c >= '0' && *c <= '9') {
            int ref = *c - '0';
            if (ref >= nops)
                error_tok_simple(node->tok, "invalid reference in constraint %d ('%c')", i, *c);
        }
    }
#endif

    tok = skip(tok, ")");
    *rest = skip(tok, ";");
    return node;
}

static Node *stmt(Token **rest, Token *tok) {
    // C23 [[attribute]] prefixing a statement or label (e.g. as an if/while
    // body, where the enclosing block loop's attribute handler does not run).
    if (equalc(tok, "[") && equalc(tok->next, "[") && tok->ptr + tok->len == tok->next->ptr) {
        tok = skip_attributes(tok);
        return stmt(rest, tok);
    }
    // contract_assert(COND[, "msg"]); / contract_assume(COND[, "msg"]);
    // Ordinary identifiers, not keywords: recognized only immediately
    // followed by '(', mirroring `defer`'s own disambiguation just below
    // (an unrelated function genuinely named contract_assert/_assume is
    // vanishingly unlikely, and this exact guard is what lets pre()/
    // post() coexist with ordinary code using those names too).
    if ((equalc(tok, "contract_assert") || equalc(tok, "contract_assume")) && equalc(tok->next, "(")) {
        bool is_assert = equalc(tok, "contract_assert");
        return parse_contract_stmt(rest, tok, is_assert);
    }
    // C23 `defer` (WG14 N3199 / TS 25755, `-fdefer-ts`): `defer
    // statement;` registers `statement` to run, LIFO with every other
    // pending cleanup/defer, when the enclosing scope exits (fall-
    // through, return, break, continue, or goto out of it). `defer {`
    // is recognized unconditionally, without needing -fdefer-ts: an
    // ordinary identifier can never be followed directly by `{` in
    // valid C (not a declaration -- `defer` is never a typedef name --
    // and not a continuable expression), so there is no real-world
    // program this could misparse. Every other spelling (`defer
    // <stmt>;` with no braces, e.g. nob.h's `defer if (f) fclose(f);`)
    // stays gated behind the flag: `defer(x);` is genuinely ambiguous
    // with a call to a function literally named `defer`.
    if (equalc(tok, "defer") && (equalc(tok->next, "{") || opt_defer_ts)) {
        // Modeled as a zero-storage marker LVar carrying the
        // already-parsed statement, synthesized directly onto the same
        // `locals` chain ordinary cleanup-attribute variables use --
        // see append_cleanup_flat()/append_cleanup_range() (parse-time
        // fall-through injection) and codegen.c's var_has_cleanup()/
        // emit_cleanup_range() (return/break/continue/goto). The
        // `defer` statement itself is a pure declaration: it contributes
        // no code at its own point, only at scope-exit sites.
        Token *defer_tok = tok;
        bool saved_in_defer = in_defer_body;
        in_defer_body = true;
        Node *body = stmt(&tok, tok->next);
        in_defer_body = saved_in_defer;
        check_type(body);
        LVar *marker = arena_alloc(sizeof(LVar));
        memset(marker, 0, sizeof(LVar));
        marker->is_local = true;
        marker->ty = ty_void;
        marker->defer_stmt = body;
        marker->next = locals;
        locals = marker;
        // Match declaration()'s own convention for an ordinary top-level
        // local (see its `if (current_block_depth == 1)
        // current_fn_scope_locals = locals;`): a defer declared directly
        // in the function's own outermost scope is exclusively fired by
        // the shared epilogue's fn->locals scan (every exit path -- both
        // real fall-through and every `return`, which all converge on
        // .L.return<fn> -- reaches it there), not duplicated by each
        // return's own emit_cleanup_range. Without this, a *later*
        // top-level declaration's own advance would silently exclude
        // this marker from any subsequent return's cleanup_begin..end
        // range, and it would never fire at all (the epilogue skips
        // defer_stmt entries it doesn't itself need to handle -- see
        // codegen.c). A defer nested inside a block (block_depth > 1)
        // is popped off `locals` when that block exits and so never
        // reaches fn->locals's final snapshot; it stays covered by
        // whichever return's own range is still in scope for it.
        if (current_block_depth == 1)
            current_fn_scope_locals = locals;
        *rest = tok;
        return new_node(ND_NULL, defer_tok);
    }
    if (equalc(tok, "return")) {
        if (in_defer_body)
            error_tok(tok, "'return' not allowed inside a 'defer' statement");
        Node *node = new_node(ND_RETURN, tok);
        node->cleanup_begin = locals;
        node->cleanup_end = current_fn_scope_locals;
        if (node->cleanup_begin != node->cleanup_end) {
            // Pending cleanup/defer code runs (arbitrary calls, clobbers
            // the ABI return register(s)) between materializing the
            // return value and the actual jump to the epilogue -- see
            // codegen.c's ND_RETURN. Reserve a scratch slot to spill it
            // into first; harmless (if unusually placed) for a void
            // return, since codegen only touches this slot when it
            // actually wrote to the return register(s). Must be
            // ty_llong, not ty_long: codegen unconditionally spills two
            // 8-byte registers into this slot, but `long` is only 4
            // bytes under Windows' LLP64 model -- `array_of(ty_long, 2)`
            // there reserved just 8 bytes for a 16-byte write, silently
            // overflowing into whatever local sat next to it on the
            // stack (found via tinycc's 101_cleanup.c test_ret2() on
            // the mingw target: a plain __attribute__((cleanup)) local
            // one stack slot over got its stored string pointer
            // clobbered by the second 8-byte spill).
            node->defer_retspill = new_var("__defer_retspill", array_of(ty_llong, 2), true);
        }
        if (equalc(tok->next, ";")) {
            *rest = tok->next->next;
            return apply_postconds_to_return(node, tok);
        }
        node->lhs = expr(&tok, tok->next);
        *rest = skip(tok, ";");
        return apply_postconds_to_return(node, tok);
    }

    if (equalc(tok, "if")) {
        Node *node = new_node(ND_IF, tok);
        tok = skip(tok->next, "(");

        if (is_typename(tok)) {
            LVar *saved_locals = locals;
            Typedef *saved_typedefs = typedefs;
            TypedefLog *saved_typedef_log = typedef_scope_checkpoint();
            EnumConst *saved_enum = enum_consts;
            EnumLog *saved_enum_log = enum_scope_checkpoint();

            // C23 if/switch init-statement: `if (init-statement; condition)`.
            // The `;` that terminates the init-statement sits at paren depth 0;
            // a function call or parenthesized subexpression inside the init
            // has its own parens, and the old flat scan stopped at the FIRST
            // `)` -- the init's own closing paren -- never seeing the `;`, so
            // `if (int num = enc(&p[x+y]); num >= 0)` was misparsed as the
            // C99 decl-in-condition form and choked on the leftover `;`.
            bool has_semi = false;
            int paren_depth = 0;
            for (Token *s = tok; s; s = s->next) {
                if (equalc(s, "(")) {
                    paren_depth++;
                } else if (equalc(s, ")")) {
                    if (paren_depth == 0)
                        break; // the condition's own closing paren
                    paren_depth--;
                } else if (paren_depth == 0 && equalc(s, ";")) {
                    has_semi = true;
                    break;
                }
            }

            if (has_semi) {
                node->init = declaration(&tok, tok);
                if (!equalc(tok, ")"))
                    node->cond = expr(&tok, tok);
            } else {
                VarAttr attr = {};
                Type *base = declspec(&tok, tok, &attr);
                char *name = NULL;
                int decl_align = 0;
                Type *ty = declarator(&tok, tok, copy_type(base), &name, &attr);
                tok = read_type_attrs(tok, &decl_align, NULL);
                if (!name)
                    error_tok(tok, "expected variable name");
                LVar *var = new_var(name, ty, true);
                Node head = {};
                Node *cur = &head;
                if (equalc(tok, "=")) {
                    tok = tok->next;
                    Node *lhs = new_var_node(var, tok);
                    tok = local_init_one(tok, lhs, var->ty, &cur);
                }
                node->init = head.next;
                node->cond = new_var_node(var, tok);
            }

            tok = skip(tok, ")");
            node->cleanup_end = saved_locals;
            node->then = stmt(&tok, tok);
            if (equalc(tok, "else"))
                node->els = stmt(&tok, tok->next);
            enum_scope_restore(saved_enum_log);
            typedef_scope_restore(saved_typedef_log);
            enum_consts = saved_enum;
            node = append_cleanup_range(node, locals, saved_locals, tok);
            locals = saved_locals;
            typedefs = saved_typedefs;
            *rest = tok;
            return node;
        }

        EnumConst *saved_enum = enum_consts;
        EnumLog *saved_enum_log = enum_scope_checkpoint();
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        if (equalc(tok, "else"))
            node->els = stmt(&tok, tok->next);
        enum_scope_restore(saved_enum_log);
        enum_consts = saved_enum;
        *rest = tok;
        return node;
    }

    if (equalc(tok, "while")) {
        Node *node = new_node(ND_FOR, tok);
        consume_pending_loop_labels(node, tok);
        tok = skip(tok->next, "(");
        EnumConst *saved_enum = enum_consts;
        EnumLog *saved_enum_log = enum_scope_checkpoint();
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        Node *saved_loop = current_loop;
        Node *saved_ctrl = current_ctrl;
        node->cleanup_end = locals;
        node->continue_cleanup_end = locals;
        node->parent_loop = saved_ctrl;
        current_loop = node;
        current_ctrl = node;
        node->then = stmt(&tok, tok);
        current_loop = saved_loop;
        current_ctrl = saved_ctrl;
        enum_scope_restore(saved_enum_log);
        enum_consts = saved_enum;
        *rest = tok;
        return node;
    }

    if (equalc(tok, "do")) {
        Node *node = new_node(ND_DO, tok);
        consume_pending_loop_labels(node, tok);
        Node *saved_loop = current_loop;
        Node *saved_ctrl = current_ctrl;
        node->cleanup_end = locals;
        node->continue_cleanup_end = locals;
        node->parent_loop = saved_ctrl;
        current_loop = node;
        current_ctrl = node;
        node->then = stmt(&tok, tok->next);
        current_loop = saved_loop;
        current_ctrl = saved_ctrl;
        tok = skip(tok, "while");
        tok = skip(tok, "(");
        EnumConst *saved_enum = enum_consts;
        EnumLog *saved_enum_log = enum_scope_checkpoint();
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        enum_scope_restore(saved_enum_log);
        enum_consts = saved_enum;
        *rest = skip(tok, ";");
        return node;
    }

    if (equalc(tok, "for")) {
        LVar *saved_locals = locals;
        Typedef *saved_typedefs = typedefs;
        TypedefLog *saved_typedef_log = typedef_scope_checkpoint();
        EnumConst *saved_enum = enum_consts;
        EnumLog *saved_enum_log = enum_scope_checkpoint();
        Node *node = new_node(ND_FOR, tok);
        consume_pending_loop_labels(node, tok);
        tok = skip(tok->next, "(");

        if (!equalc(tok, ";")) {
            if (is_typename(tok) || equalc(tok, "_Static_assert") ||
                equalc(tok, "static_assert")) {
                if (equalc(tok, "_Static_assert") || equalc(tok, "static_assert")) {
                    Token *s = tok->next->next;
                    while (s && !equalc(s, ",") && !equalc(s, ")")) {
                        if (equalc(s, "{")) {
                            error_tok(s, "declaration of static_assert in for loop declares a type");
                            break;
                        }
                        s = s->next;
                    }
                }
                node->init = declaration(&tok, tok);
            } else {
                node->init = expr(&tok, tok);
                tok = skip(tok, ";");
            }
        } else {
            tok = tok->next;
        }

        if (!equalc(tok, ";"))
            node->cond = expr(&tok, tok);
        tok = skip(tok, ";");

        LVar *for_init_locals = locals;

        if (!equalc(tok, ")"))
            node->inc = expr(&tok, tok);
        tok = skip(tok, ")");
        Node *saved_loop = current_loop;
        Node *saved_ctrl = current_ctrl;
        node->cleanup_end = for_init_locals;
        node->continue_cleanup_end = for_init_locals;
        node->parent_loop = saved_ctrl;
        current_loop = node;
        current_ctrl = node;
        node->then = stmt(&tok, tok);
        current_loop = saved_loop;
        current_ctrl = saved_ctrl;
        enum_scope_restore(saved_enum_log);
        typedef_scope_restore(saved_typedef_log);
        enum_consts = saved_enum;
        node = append_cleanup_range(node, locals, saved_locals, tok);
        locals = saved_locals;
        typedefs = saved_typedefs;
        *rest = tok;
        return node;
    }

    if (equalc(tok, "switch")) {
        Node *node = new_node(ND_SWITCH, tok);
        tok = skip(tok->next, "(");

        if (is_typename(tok)) {
            LVar *saved_locals = locals;
            Typedef *saved_typedefs = typedefs;
            TypedefLog *saved_typedef_log = typedef_scope_checkpoint();
            EnumConst *saved_enum = enum_consts;
            EnumLog *saved_enum_log = enum_scope_checkpoint();

            // C23 if/switch init-statement: `if (init-statement; condition)`.
            // The `;` that terminates the init-statement sits at paren depth 0;
            // a function call or parenthesized subexpression inside the init
            // has its own parens, and the old flat scan stopped at the FIRST
            // `)` -- the init's own closing paren -- never seeing the `;`, so
            // `if (int num = enc(&p[x+y]); num >= 0)` was misparsed as the
            // C99 decl-in-condition form and choked on the leftover `;`.
            bool has_semi = false;
            int paren_depth = 0;
            for (Token *s = tok; s; s = s->next) {
                if (equalc(s, "(")) {
                    paren_depth++;
                } else if (equalc(s, ")")) {
                    if (paren_depth == 0)
                        break; // the condition's own closing paren
                    paren_depth--;
                } else if (paren_depth == 0 && equalc(s, ";")) {
                    has_semi = true;
                    break;
                }
            }

            if (has_semi) {
                node->init = declaration(&tok, tok);
                if (!equalc(tok, ")")) {
                    node->cond = expr(&tok, tok);
                    check_type(node->cond);
                    if (node->cond->ty && !is_integer(node->cond->ty))
                        error_tok(node->cond->tok,
                                  "switch quantity not an integer");
                }
            } else {
                VarAttr attr = {};
                Type *base = declspec(&tok, tok, &attr);
                char *name = NULL;
                int decl_align = 0;
                Type *ty = declarator(&tok, tok, copy_type(base), &name, &attr);
                tok = read_type_attrs(tok, &decl_align, NULL);
                if (!name)
                    error_tok(tok, "expected variable name");
                LVar *var = new_var(name, ty, true);
                Node head = {};
                Node *cur = &head;
                if (equalc(tok, "=")) {
                    tok = tok->next;
                    Node *lhs = new_var_node(var, tok);
                    tok = local_init_one(tok, lhs, var->ty, &cur);
                }
                node->init = head.next;
                node->cond = new_var_node(var, tok);
                if (!is_integer(var->ty))
                    error_tok(node->cond->tok, "switch quantity not an integer");
            }

            tok = skip(tok, ")");
            node->cleanup_end = saved_locals;
            Node *saved = current_switch;
            Node *saved_ctrl = current_ctrl;
            consume_pending_loop_labels(node, tok);
            node->parent_loop = saved_ctrl;
            current_switch = node;
            current_ctrl = node;
            node->then = stmt(&tok, tok);
            current_switch = saved;
            current_ctrl = saved_ctrl;
            enum_scope_restore(saved_enum_log);
            typedef_scope_restore(saved_typedef_log);
            enum_consts = saved_enum;
            node = append_cleanup_range(node, locals, saved_locals, tok);
            locals = saved_locals;
            typedefs = saved_typedefs;
            *rest = tok;
            return node;
        }

        EnumConst *saved_enum = enum_consts;
        EnumLog *saved_enum_log = enum_scope_checkpoint();
        node->cond = expr(&tok, tok);
        check_type(node->cond);
        if (node->cond->ty && !is_integer(node->cond->ty))
            error_tok(node->cond->tok, "switch quantity not an integer");
        tok = skip(tok, ")");
        Node *saved = current_switch;
        Node *saved_ctrl = current_ctrl;
        consume_pending_loop_labels(node, tok);
        node->cleanup_end = locals;
        node->parent_loop = saved_ctrl;
        current_switch = node;
        current_ctrl = node;
        node->then = stmt(&tok, tok);
        current_switch = saved;
        current_ctrl = saved_ctrl;
        enum_scope_restore(saved_enum_log);
        enum_consts = saved_enum;
        *rest = tok;
        return node;
    }

    if (equalc(tok, "case")) {
        if (!current_switch)
            error_tok(tok, "stray case label");
        Node *node = new_node(ND_CASE, tok);
        tok = tok->next;
        Node *val_node = conditional(&tok, tok);
        check_type(val_node);
        long long v = 0;
        if ((val_node->ty && val_node->ty->kind == TY_NULLPTR_T) ||
            !eval_const_expr(val_node, &v))
            error_tok(val_node->tok, "case label is not an integer constant expression");
        node->case_val = v;
        if (equalc(tok, "...")) {
            tok = tok->next;
            Node *end_node = conditional(&tok, tok);
            check_type(end_node);
            long long ev = 0;
            if ((end_node->ty && end_node->ty->kind == TY_NULLPTR_T) ||
                !eval_const_expr(end_node, &ev))
                error_tok(end_node->tok, "case range is not an integer constant expression");
            node->case_end = ev;
            node->is_case_range = true;
        }
        tok = skip(tok, ":");
        // C23: a label may be immediately followed by a declaration or the
        // end of the enclosing compound statement, with an empty (null)
        // statement body - see the identical exception for plain
        // identifier labels above.
        if (equalc(tok, "}") || equalc(tok, "__auto_type") ||
            equalc(tok, "_Static_assert") || equalc(tok, "static_assert") ||
            is_typename(tok))
            node->lhs = new_node(ND_NULL, tok);
        else
            node->lhs = stmt(&tok, tok);
        node->case_next = current_switch->case_next;
        current_switch->case_next = node;
        *rest = tok;
        return node;
    }

    if (equalc(tok, "default")) {
        if (!current_switch)
            error_tok(tok, "stray default label");
        Node *node = new_node(ND_CASE, tok);
        node->case_val = -1;
        tok = skip(tok->next, ":");
        if (equalc(tok, "}") || equalc(tok, "__auto_type") ||
            equalc(tok, "_Static_assert") || equalc(tok, "static_assert") ||
            is_typename(tok))
            node->lhs = new_node(ND_NULL, tok);
        else
            node->lhs = stmt(&tok, tok);
        current_switch->default_case = node;
        *rest = tok;
        return node;
    }


    if (equalc(tok, "break")) {
        Node *node = new_node(ND_BREAK, tok);
        node->cleanup_begin = locals;
        if (tok->next->kind == TK_IDENT) {
            // C2Y labeled break: must name a loop or switch enclosing this
            // statement; the break exits that statement, not the innermost
            // one. Under -std=c23 this is a C2Y feature (pedantic error).
            node->label_name = tok->next->name;
            tok = tok->next->next;
            *rest = skip(tok, ";");
            for (Node *l = current_ctrl; l; l = l->parent_loop) {
                if (node_has_label(l, node->label_name)) {
                    node->target_loop = l;
                    break;
                }
            }
            if (opt_pedantic && !opt_Wno_c23_c2y_compat)
                warn_tok(tok, "ISO C does not support 'break' statement with an identifier operand before C2Y");
            if (!node->target_loop)
                error_tok(tok, "break label '%s' is not a loop or switch label",
                          node->label_name);
            node->cleanup_end = node->target_loop->cleanup_end;
            return node;
        }
        *rest = skip(tok->next, ";");
        if (current_switch) {
            node->cleanup_end = current_switch->cleanup_end;
            return node;
        }
        if (current_loop) {
            node->cleanup_end = current_loop->cleanup_end;
            return node;
        }
        error_tok(tok, "stray break");
    }

    if (equalc(tok, "continue")) {
        Node *node = new_node(ND_CONTINUE, tok);
        node->cleanup_begin = locals;
        if (tok->next->kind == TK_IDENT) {
            // C2Y labeled continue: must name a loop enclosing this
            // statement; the continue jumps to that loop's continuation
            // (skipping everything between, including inner loop bodies).
            // The old code dropped the label and treated the statement as
            // a plain continue of the innermost loop.
            node->label_name = tok->next->name;
            tok = tok->next->next;
            *rest = skip(tok, ";");
            for (Node *l = current_ctrl; l; l = l->parent_loop) {
                if (node_has_label(l, node->label_name)) {
                    if (l->kind == ND_FOR || l->kind == ND_DO) {
                        node->target_loop = l;
                        break;
                    }
                    /* a switch carries the label: continue is illegal there */
                    error_tok(tok, "continue label '%s' does not label a loop",
                              node->label_name);
                }
            }
            if (opt_pedantic && !opt_Wno_c23_c2y_compat)
                warn_tok(tok, "ISO C does not support 'continue' statement with an identifier operand before C2Y");
            if (!node->target_loop)
                error_tok(tok, "continue label '%s' does not label an enclosing loop",
                          node->label_name);
            node->cleanup_end = node->target_loop->continue_cleanup_end;
            return node;
        }
        *rest = skip(tok->next, ";");
        if (!current_loop)
            error_tok(tok, "stray continue");
        node->cleanup_end = current_loop->continue_cleanup_end;
        return node;
    }

    if (equalc(tok, "goto")) {
        tok = tok->next;
        if (equalc(tok, "*")) {
            // codeql[cpp/commented-out-code]: doc comment showing the GNU computed-goto syntax parsed below, not dead code
            // Computed goto: goto *expr;
            Node *node = new_node(ND_GOTO_IND, tok);
            tok = tok->next;
            node->lhs = expr(&tok, tok);
            *rest = skip(tok, ";");
            return node;
        }
        Node *node = new_node(ND_GOTO, tok);
        if (tok->kind != TK_IDENT)
            error_tok(tok, "expected label name");
        node->label_name = tok->name;
        node->cleanup_begin = locals;
        LabelScope *label = find_label_scope(node->label_name);
        node->cleanup_end = label ? label->locals : current_fn_scope_locals;
        if (!label) {
            int owner_depth;
            char *owner_fn = find_label_scope_owner(node->label_name, &owner_depth);
            if (owner_fn) {
                // Nonlocal goto: the label is declared (via __label__) in an
                // enclosing function, not this one. Codegen restores that
                // ancestor frame's rbp/rsp (chain-walking chain_depth
                // levels, mirroring variable capture) before jumping —
                // see codegen.c's ND_GOTO case.
                node->funcname = owner_fn;
                node->chain_depth = owner_depth;
                mark_goto_target_fn(owner_fn);
            } else {
                add_pending_goto(node->label_name, node);
            }
        }
        *rest = skip(tok->next, ";");
        return node;
    }

    if (tok->kind == TK_IDENT && equalc(tok->next, ":")) {
        Node *node = new_node(ND_LABEL, tok);
        node->label_name = tok->name;
        record_label_scope(node->label_name, locals);
        resolve_pending_gotos(node->label_name, locals);
        tok = tok->next->next;
        Token *before_attrs = tok;
        tok = skip_attributes(tok);
        // Declarations, including attribute declarations, cannot appear
        // after labels when a statement is expected. (C23 [[]] attrs only)
        if (tok != before_attrs && equalc(tok, ";") &&
            equalc(before_attrs, "[") && before_attrs->next &&
            equalc(before_attrs->next, "[") &&
            before_attrs->ptr + before_attrs->len == before_attrs->next->ptr)
            error_tok(tok, "expected statement before attribute declaration");
        // C23: a label may immediately precede a declaration, or appear at the
        // end of a compound statement (before '}').  In those cases the label
        // has an empty (null) statement body and the enclosing block parses the
        // declaration, if any, as the next block item.
        if (equalc(tok, "}") || equalc(tok, "__auto_type") ||
            equalc(tok, "_Static_assert") || equalc(tok, "static_assert") ||
            is_typename(tok)) {
            /* No loop/switch follows: drop any accumulated label chain so a
             * later loop elsewhere doesn't wrongly claim these labels. */
            pending_loop_labels_n = 0;
            node->lhs = new_node(ND_NULL, tok);
            *rest = tok;
            return node;
        }
        // C2Y 6.8.7: a label directly preceding a loop or switch is that
        // statement's label, the target of `continue label;` (loops) /
        // `break label;` (loops and switches). Accumulate the name into the
        // pending chain so a chain like `a: b: c: for (...)` labels the
        // loop with all three (the loop node must carry them while its body
        // is parsed, so labeled continue/break inside the body can resolve
        // them); the next loop/switch parse consumes the whole chain, and
        // any other statement clears it.
        if (pending_loop_labels_n < 32)
            pending_loop_labels[pending_loop_labels_n++] = node->label_name;
        node->lhs = stmt(&tok, tok);
        if (!(node->lhs->kind == ND_FOR || node->lhs->kind == ND_DO ||
              node->lhs->kind == ND_SWITCH))
            pending_loop_labels_n = 0; /* label not on a loop/switch */
        *rest = tok;
        return node;
    }

    if (equalc(tok, "{"))
        return compound_stmt(rest, tok);

    if (equalc(tok, ";")) {
        *rest = tok->next;
        return new_node(ND_NULL, tok);
    }

    if (is_asm_keyword(tok))
        return parse_asm_stmt(rest, tok);

    // Standalone __attribute__((...)) statement (e.g., __fallthrough__)
    if (equalc(tok, "__attribute__") || equalc(tok, "__attribute")) {
        tok = skip_attributes(tok);
        *rest = skip(tok, ";");
        return new_node(ND_NULL, tok);
    }


    // _Pragma("string") — C99 pragma operator, treat as no-op
    if (equalc(tok, "_Pragma")) {
        tok = tok->next;
        tok = skip(tok, "(");
        if (tok->kind == TK_STR)
            tok = tok->next;
        tok = skip(tok, ")");
        *rest = tok;
        return new_node(ND_NULL, tok);
    }
    Node *node = new_node(ND_EXPR_STMT, tok);
    node->lhs = expr(&tok, tok);
    // Type-check eagerly, during parsing, like virtually every other
    // statement-constructing path already does (declarations,
    // compound-assignment, casts, switch/ternary, ...) -- not deferred to
    // main.c's post-parse "Type system / Semantic checks" pass, which is
    // unconditionally skipped for the whole file the moment ANY earlier
    // error was collected (GH #34: "the AST is incomplete, so skip
    // typecheck/codegen for this file"). A bare expression statement
    // (`m = 1;`, not a declaration) was the one construct relying solely
    // on that best-effort pass, so any constraint violation in it went
    // completely undiagnosed whenever the file had an earlier, unrelated
    // error -- e.g. a nullptr_t assignment several statements after some
    // other mistake silently compiled instead of being reported.
    check_type(node->lhs);
    *rest = skip(tok, ";");
    return node;
}

static bool type_equal(Type *a, Type *b) {
    if (a == b)
        return true;
    if (!a || !b)
        return false;
    // C11 6.2.7p1 array-type compatibility: element types must be
    // compatible, and array size only has to match when BOTH sides
    // specify a constant size — an incomplete/unsized array `T[]` (size
    // 0) is compatible with any sized `T[N]`, and either side being a
    // VLA (runtime-computed size, e.g. the `vec2array()`-style macros
    // noplate uses, whose array size is `vec_length(...)`) makes the
    // length irrelevant entirely. type_equal() backs both _Generic's
    // association matching (C11 6.5.1.1p2, itself defined via
    // "compatible type") and redeclaration-conflict checking — both need
    // this relaxation, so it must run before the strict a->kind !=
    // b->kind check below (a fixed array is comparable against a VLA of
    // the same element type). types_compatible_p_qual() below already
    // implements the identical rule; this mirrors it. Real bug: noplate's
    // TYPE_CHECK(typeof(x[0])(*)[], &x) idiom always missed via
    // _Generic, both on the unsized-`[]` side and the VLA side.
    bool a_arr = a->kind == TY_ARRAY || a->kind == TY_VLA;
    bool b_arr = b->kind == TY_ARRAY || b->kind == TY_VLA;
    if (a_arr || b_arr) {
        if (!a_arr || !b_arr)
            return false;
        if (a->qual != b->qual)
            return false;
        if (!type_equal(a->base, b->base))
            return false;
        if (a->kind == TY_VLA || b->kind == TY_VLA)
            return true;
        return a->size == 0 || b->size == 0 || a->size == b->size;
    }
    if (a->kind != b->kind)
        return false;
    if (a->qual != b->qual)
        return false;
    if (a->is_unsigned != b->is_unsigned)
        return false;
    if (a->kind == TY_CHAR && a->is_signed_char != b->is_signed_char)
        return false;
    if (a->is_variadic != b->is_variadic)
        return false;

    switch (a->kind) {
    case TY_COMPLEX:
        return type_equal(a->base, b->base);
    case TY_PTR:
        return type_equal(a->base, b->base);
    case TY_FUNC:
        if (!type_equal(a->return_ty, b->return_ty))
            return false;
        {
            Type *pa = a->param_types;
            Type *pb = b->param_types;
            if (!pa || !pb)
                return true;
            while (pa && pb) {
                // C11 6.7.6.3p15: a parameter's own top-level qualifiers
                // don't affect function-type compatibility (they only
                // constrain the callee's local copy) - unlike qualifiers
                // nested deeper (e.g. a `const char *` parameter, where the
                // qualifier is on what's pointed to, not the parameter
                // itself). Compare unqualified shallow copies so `const
                // double` and `double` parameters - and nested cases like
                // `int(const double)` vs `int(*)(double)`, where this
                // param comparison recurses into the inner function type's
                // own parameter list - are recognized as compatible.
                if (pa->qual != pb->qual) {
                    Type qa = *pa, qb = *pb;
                    qa.qual = 0;
                    qb.qual = 0;
                    if (!type_equal(&qa, &qb))
                        return false;
                } else if (!type_equal(pa, pb))
                    return false;
                pa = pa->param_next;
                pb = pb->param_next;
            }
            return !pa && !pb;
        }
    case TY_BITINT:
        return a->bitint_width == b->bitint_width;
    case TY_STRUCT:
    case TY_UNION:
        // Parameter-list nodes are shallow copies (`*pt = *pty` in
        // declarator_params) of the canonical tag's Type, so a struct/union
        // parameter re-parsed from a second declaration (e.g. the function
        // definition) is never pointer-identical to the first — even more
        // so when nested inside a function-pointer parameter's own param
        // list, which is copied again. `members` is preserved unmodified
        // by the shallow copy and is set once, when the tag is defined, so
        // comparing it is stable across copies while still distinguishing
        // different tags (e.g. two same-shaped anonymous structs used as
        // distinct _Generic selectors). A bare incomplete struct/union
        // can't appear here: parameters require complete types.
        return a->members == b->members;
    default:
        return true;
    }
}

// Qualifier-aware structural type compatibility, used recursively by
// types_compatible_p() below for everything EXCEPT the two top-level
// argument types themselves (whose own qualifiers GCC's
// __builtin_types_compatible_p explicitly disregards - "const int" and
// "int" are compatible). Nested qualifiers (what a pointer points to, an
// array's element, a struct member) still count normally.
static bool types_compatible_p_qual(Type *a, Type *b) {
    if (!a || !b) return false;
    if (a->qual != b->qual) return false;
    // C11 6.7.6.2p6-7: array types (including VLAs) are compatible if
    // their element types are compatible and, when both size specifiers
    // are present AND both are compile-time constants, they have the same
    // value. A VLA's runtime length is never a compile-time compatibility
    // criterion, so either side being TY_VLA (or an unspecified-length
    // array, size 0) makes length irrelevant. Handle TY_ARRAY/TY_VLA here,
    // before the strict a->kind != b->kind check below, so a fixed array
    // is comparable against a VLA of the same element type.
    bool a_arr = a->kind == TY_ARRAY || a->kind == TY_VLA;
    bool b_arr = b->kind == TY_ARRAY || b->kind == TY_VLA;
    if (a_arr || b_arr) {
        if (!a_arr || !b_arr) return false;
        if (!types_compatible_p_qual(a->base, b->base)) return false;
        if (a->kind == TY_VLA || b->kind == TY_VLA) return true;
        // Both TY_ARRAY: array_of() never populates ->array_len (only
        // vla_of()'s constant-VLA fallback does), so the real element
        // count comes from ->size / ->base->size.
        int64_t la = a->base->size ? a->size / a->base->size : 0;
        int64_t lb = b->base->size ? b->size / b->base->size : 0;
        return la == 0 || lb == 0 || la == lb;
    }
    if (a->kind != b->kind) return false;
    if (a->is_unsigned != b->is_unsigned) return false;
    if (a->kind == TY_CHAR && a->is_signed_char != b->is_signed_char) return false;
    if (a->is_variadic != b->is_variadic) return false;
    // Two `enum` types are only compatible if they're literally the same
    // declaration (C 6.2.7): identical size/signedness is not enough — two
    // separately declared enums (even with identical enumerator values, or
    // an anonymous `enum {...}` structurally matching a named one) are
    // always incompatible. An enum compared against a *non*-enum integer
    // type of matching kind/size/signedness (e.g. `enum E1 : short` vs
    // plain `short`) still falls through to the representation-only checks
    // below — only enum-vs-enum needs the identity check.
    if (a->is_enum && b->is_enum)
        return a->enum_id && a->enum_id == b->enum_id;
    switch (a->kind) {
    case TY_PTR:
    case TY_COMPLEX:
        return types_compatible_p_qual(a->base, b->base);
    case TY_FUNC: {
        if (!types_compatible_p_qual(a->return_ty, b->return_ty)) return false;
        Type *pa = a->param_types, *pb = b->param_types;
        // Unlike type_equal() (used for real prototype/definition
        // redeclaration merging, where "no parameter info" genuinely means
        // a K&R old-style declarator compatible with anything), a bare
        // "()" reaching this function is always a type-NAME - there is no
        // way to write actual K&R syntax without parameter names, which a
        // type name never has - so it's treated as exactly zero
        // parameters, matching GCC's __builtin_types_compatible_p(void(),
        // void(int)) => incompatible.
        while (pa && pb) {
            // C11 6.7.6.3p15: a parameter's own top-level qualifiers don't
            // affect function-type compatibility.
            Type qa = *pa, qb = *pb;
            qa.qual = 0;
            qb.qual = 0;
            if (!types_compatible_p_qual(&qa, &qb)) return false;
            pa = pa->param_next;
            pb = pb->param_next;
        }
        return !pa && !pb;
    }
    case TY_STRUCT:
    case TY_UNION:
        // See type_equal()'s identical case above for why pointer/member-
        // list identity (not qual/size) is the right comparison here.
        // struct_id additionally recognizes two qualified variants of the
        // SAME still-incomplete tag (Type.qual_variants) as compatible --
        // qualify_struct_type() mints a fresh Type object at every
        // `const struct S *`-style use site while S stays forward-
        // declared, so neither pointer identity nor the (both-NULL)
        // member-list check can tell they're the same tag.
        return (a == b) || (a->members && a->members == b->members) ||
            (a->struct_id && a->struct_id == b->struct_id);
    case TY_BITINT:
        return a->bitint_width == b->bitint_width;
    default:
        return a->size == b->size;
    }
}

// __builtin_types_compatible_p(t1, t2): GCC/Clang builtin used throughout
// real-world C (container_of()-style static_assert(__same_type(...)) checks,
// _Generic-less type dispatch). Disregards qualifiers on the two top-level
// argument types only; everything nested is qualifier-aware structural
// compatibility via types_compatible_p_qual() above.
static bool types_compatible_p(Type *a, Type *b) {
    if (!a || !b) return false;
    Type qa = *a, qb = *b;
    qa.qual = 0;
    qb.qual = 0;
    return types_compatible_p_qual(&qa, &qb);
}

static Node *apply_postfix_ops(Node *node, Token **rest, Token *tok);

static Node *primary(Token **rest, Token *tok) {
    Node *node = NULL;

    if (equalc(tok, "_Generic")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Type *ctrl_ty;
        bool ctrl_is_type = is_typename(tok);
        if (ctrl_is_type) {
            // C2Y / GCC extension: the controlling operand may be a type name.
            // No lvalue conversion is applied, so qualifiers are preserved
            // (e.g. _Generic(const int, int:1, const int:2) selects const int).
            // Associations may then be incomplete/function types.
            ctrl_ty = type_name(&tok, tok);
        } else {
            Node *ctrl = assign(&tok, tok);
            check_type(ctrl);
            ctrl_ty = ctrl->ty;
            // Apply lvalue/array/function decay
            if (ctrl_ty->kind == TY_ARRAY)
                ctrl_ty = decay_to_ptr(ctrl_ty);
            else if (ctrl_ty->kind == TY_FUNC)
                ctrl_ty = pointer_to(ctrl_ty);
            // Lvalue conversion strips top-level qualifiers
            if (ctrl_ty->qual) {
                ctrl_ty = copy_type(ctrl_ty);
                ctrl_ty->qual = 0;
            }
        }

        tok = skip(tok, ",");

        Node *selected = NULL;
        Node *default_expr = NULL;
        // C11 6.5.1.1 constraints on the association list
        Type *assoc_types[64];
        int n_assoc = 0;
        while (!equalc(tok, ")")) {
            if (equalc(tok, "default")) {
                if (default_expr)
                    error_tok(tok, "duplicate 'default' case in _Generic");
                tok = skip(tok->next, ":");
                default_expr = assign(&tok, tok);
            } else {
                Token *aty_tok = tok;
                Type *ty = type_name(&tok, tok);
                if (!ctrl_is_type) {
                    // C11 6.5.1.1: expression-controlled associations must be
                    // complete object types other than VLAs.
                    if (ty->kind == TY_FUNC)
                        error_tok(aty_tok, "_Generic association has function type");
                    if (ty->kind == TY_VLA)
                        error_tok(aty_tok, "_Generic association has variable length type");
                    if (ty->kind == TY_VOID ||
                        ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) &&
                         !ty->has_body))
                        error_tok(aty_tok, "_Generic association has incomplete type");
                }
                // Duplicate-type check on scalar non-pointer types only:
                // type_equal() is deliberately loose on pointer/function
                // shapes, which would yield false duplicates here.
                if (ty->kind != TY_PTR && ty->kind != TY_FUNC)
                    for (int i = 0; i < n_assoc; i++)
                        if (assoc_types[i]->kind == ty->kind &&
                            type_equal(assoc_types[i], ty) &&
                            assoc_types[i]->qual == ty->qual)
                            error_tok(aty_tok,
                                      "_Generic: two compatible types in association list");
                if (n_assoc < 64)
                    assoc_types[n_assoc++] = ty;
                tok = skip(tok, ":");
                Node *expr = assign(&tok, tok);
                // Association types must match the controlling type exactly,
                // including top-level qualifiers (which matters when the
                // controlling operand is a qualified type name).
                if (type_equal(ctrl_ty, ty) && ctrl_ty->qual == ty->qual)
                    selected = expr;
            }
            if (equalc(tok, ","))
                tok = tok->next;
        }

        if (!selected && default_expr)
            selected = default_expr;
        if (!selected)
            error_tok(start, "_Generic: no matching association");

        tok = skip(tok, ")");
        node = selected;
    } else if (equalc(tok, "(")) {
        if (equalc(tok->next, "{")) {
            node = new_node(ND_STMT_EXPR, tok);
            LVar *block_locals = NULL;
            Node *block = compound_stmt_ex(&tok, tok->next, &block_locals, true);
            node->body = block->body;
            // Find result BEFORE cleanup nodes are appended
            Node *last = node->body;
            while (last && last->next)
                last = last->next;
            if (last && last->kind == ND_EXPR_STMT && last->lhs)
                node->stmt_expr_result = last->lhs;
            // Append cleanups as a flat list (block_locals → locals = saved after block)
            node->body = append_cleanup_flat(node->body, block_locals, locals, tok);
            tok = skip(tok, ")");
        } else {
            node = expr(&tok, tok->next);
            tok = skip(tok, ")");
        }
    } else if (tok->kind == TK_IDENT) {
        // __builtin_has_attribute(expr, attr_name) — compile-time attribute check
        if (equalc(tok, "__builtin_has_attribute")) {
            tok = tok->next;
            tok = skip(tok, "(");
            Node *arg = assign(&tok, tok);
            check_type(arg);
            tok = skip(tok, ",");
            char *attr_name = NULL;
            if (tok->kind == TK_IDENT)
                attr_name = tok->name;
            tok = tok->next;
            tok = skip(tok, ")");
            int result = 0;
            // Walk through dereferences to get the function type
            Node *fn_node = arg;
            while (fn_node && fn_node->kind == ND_DEREF)
                fn_node = fn_node->lhs;
            // Get the function type from the expression's type
            Type *fn_ty = arg->ty;
            while (fn_ty && fn_ty->kind == TY_PTR)
                fn_ty = fn_ty->base;
            // For conditional expressions, the composite type already has
            // merged C23 attributes — check the type directly
            if (fn_ty && fn_ty->kind == TY_FUNC && attr_name) {
                if (strcmp(attr_name, "reproducible") == 0 && fn_ty->is_reproducible)
                    result = 1;
                else if (strcmp(attr_name, "unsequenced") == 0 && fn_ty->is_unsequenced)
                    result = 1;
            }
            // Also check LVar for direct function references (not via type)
            if (!result && fn_node && fn_node->kind == ND_LVAR && fn_node->var) {
                LVar *v = fn_node->var;
                if (strcmp(attr_name, "reproducible") == 0 && v->is_function && v->is_reproducible)
                    result = 1;
                else if (strcmp(attr_name, "unsequenced") == 0 && v->is_function && v->is_unsequenced)
                    result = 1;
            }
            node = new_num(result, tok);
            *rest = tok;
            return node;
        }
        // __has_attribute(attr_name) — GNU/Clang extension, also usable
        // (unlike __has_include/__has_c_attribute) as a genuine
        // compile-time constant expression in ordinary code, not just
        // inside #if (e.g. linux/kernel/trace/trace.c: "if (... &&
        // __has_attribute(btf_type_tag)) return;"). Reports whether this
        // compiler recognizes AND acts upon the named __attribute__,
        // mirroring the set __attribute__((...)) parsing actually
        // dispatches on above — every other syntactically-valid attribute
        // name is silently accepted-but-ignored, so reporting 1 for those
        // too would misrepresent them as semantically implemented.
        if (equalc(tok, "__has_attribute")) {
            tok = tok->next;
            tok = skip(tok, "(");
            char *attr_name = NULL;
            if (tok->kind == TK_IDENT)
                attr_name = tok->name;
            tok = tok->next;
            tok = skip(tok, ")");
            static const char *const known_attrs[] = {
                "alias",
                "aligned",
                "cleanup",
                "const",
                "constructor",
                "deprecated",
                "destructor",
                "diagnose_if",
                "error",
                "gcc_struct",
                "gnu_inline",
                "mode",
                "ms_struct",
                "noreturn",
                "packed",
                "pure",
                "reproducible",
                "unsequenced",
                "vector_size",
                "warning",
                "weak",
                NULL,
            };
            int result = 0;
            if (attr_name)
                for (int i = 0; known_attrs[i]; i++)
                    if (!strcmp(attr_name, known_attrs[i])) {
                        result = 1;
                        break;
                    }
            node = new_num(result, tok);
            *rest = tok;
            return node;
        }
        // __FUNCTION__, __func__, __PRETTY_FUNCTION__ → current function name string
        if (equalc(tok, "__FUNCTION__") || equalc(tok, "__func__") || equalc(tok, "__PRETTY_FUNCTION__")) {
            const char *fn = parser_current_fn ? parser_current_fn : "";
            node = new_node(ND_STR, tok);
            node->ty = array_of(ty_char, strlen(fn) + 1);
            StrLit *s = new_str_lit((char *)fn, strlen(fn), 0, 1);
            node->str_id = s->id;
            *rest = tok->next;
            return node;
        }
        // __builtin___clear_cache(begin, end): flush the instruction cache
        // so newly-written (JIT-generated) code is visible to the CPU's
        // instruction fetch unit. rcc doesn't hand-encode the
        // (architecture-specific) cache-invalidation instruction sequence
        // itself; instead, like GCC's own ports for targets it doesn't
        // inline this on, redirect to the real `__clear_cache(char*,
        // char*)` runtime function libgcc already provides -- rcc's
        // linker driver invokes the system's real `gcc` to link, which
        // always pulls in libgcc, so this resolves on every target rcc
        // supports. Verified on x86-64 against real GCC: a bare call
        // compiles to nothing there (the architecture's instruction
        // cache is coherent), and libgcc's own __clear_cache is equally
        // a no-op on that target, so redirecting is correct everywhere,
        // not just an x86-64-specific shortcut.
        // Found via a real PHP build: ext/opcache/jit/ir/ir.c's
        // ir_mem_flush(), which previously linked as "undefined
        // reference to `__builtin___clear_cache`" -- rcc has no special
        // recognition for that literal name at all, so it fell through
        // to an ordinary (never-defined) direct call.
        if (equalc(tok, "__builtin___clear_cache") && equalc(tok->next, "(")) {
            Token *start = tok;
            tok = skip(tok->next, "(");
            Node *begin = assign(&tok, tok);
            check_type(begin);
            tok = skip(tok, ",");
            Node *end = assign(&tok, tok);
            check_type(end);
            *rest = skip(tok, ")");
            Node *fn = new_node(ND_FUNCALL, start);
            fn->funcname = str_intern("__clear_cache", 13);
            fn->args = begin;
            begin->next = end;
            fn->ty = ty_void;
            return fn;
        }
        // __builtin_clear_padding(ptr) — zero all padding bytes via memset
        if (equalc(tok, "__builtin_clear_padding")) {
            tok = skip(tok->next, "(");
            Node *ptr = assign(&tok, tok);
            check_type(ptr);
            *rest = skip(tok, ")");
            if (ptr && ptr->ty && ptr->ty->kind == TY_PTR && ptr->ty->base && ptr->ty->base->size > 0) {
                Node *sz = new_num(ptr->ty->base->size, tok);
                sz->ty = ty_ulong;
                Node *zero = new_num(0, tok);
                zero->ty = ty_int;
                Node *fn = new_node(ND_FUNCALL, tok);
                fn->funcname = str_intern("memset", 6);
                fn->args = ptr;
                ptr->next = zero;
                zero->next = sz;
                fn->ty = pointer_to(ty_void);
                return fn;
            }
            return new_node(ND_NULL, tok);
        }
        if (equalc(tok->next, "(")) {
            Token *fn_tok = tok;
            node = new_node(ND_FUNCALL, tok);
            LVar *var = find_var(tok);
            if (var) {
                node->lhs = new_var_node(var, tok);
                node->lhs->chain_depth = last_find_var_chain_depth;
            } else {
                node->funcname = tok->name;
                LVar *gvar = find_global_name(tok->name);
                if (gvar && gvar->is_function)
                    node->lhs = new_var_node(gvar, tok);
                else if (tok->len > 10 && !memcmp(tok->ptr, "__builtin_", 10))
                    node->lhs = declare_builtin_on_demand(tok);
            }
            tok = skip(tok->next, "(");
            Node head = {};
            Node *cur = &head;
            while (!equalc(tok, ")")) {
                if (cur != &head)
                    tok = skip(tok, ",");
                cur = cur->next = assign(&tok, tok);
            }
            node->args = head.next;
            tok = skip(tok, ")");
            cast_funcall_args(node);
            // creal/crealf/creall and cimag/cimagf/cimagl are pure component
            // loads. glibc declares them as real libm functions, and calling
            // them with a _Complex long double argument is an ABI trap: SysV
            // passes the 32-byte value on the stack as x87 80-bit components,
            // which rcc neither pushes nor represents (its internal long
            // double is a plain double in a 16-byte slot), so libm's
            // creall/cimagl returned garbage — mpc's mpc_set_ldc/get_ldc
            // produced @NaN@ values. Lower them inline like the __builtin_
            // forms. The names are reserved (C11 7.31.7) and glibc's
            // prototype (via complex.h) resolves to the same libm functions,
            // so rewriting regardless of the declared symbol is safe; a
            // caller passing anything but a complex value gets the standard
            // diagnostic from the ND_REAL/ND_IMAG typing.
            // fn_tok->name, not node->funcname: the latter is only set for
            // undeclared calls — complex.h's prototype resolves creall to a
            // global LVar, leaving funcname NULL.
            if (fn_tok->name && node->args && !node->args->next &&
                (!strcmp(fn_tok->name, "creal") ||
                 !strcmp(fn_tok->name, "crealf") ||
                 !strcmp(fn_tok->name, "creall") ||
                 !strcmp(fn_tok->name, "cimag") ||
                 !strcmp(fn_tok->name, "cimagf") ||
                 !strcmp(fn_tok->name, "cimagl"))) {
                bool is_imag = fn_tok->name[0] == 'c' && fn_tok->name[1] == 'i';
                node = new_unary(is_imag ? ND_IMAG : ND_REAL, node->args, fn_tok);
                check_type(node);
            }
            if (!var || !var->is_local) {
                InlinePackFn *ipf = find_inline_pack_fn(fn_tok->name);
                if (ipf)
                    node = inline_pack_call(node, ipf, fn_tok);
            }
        } else {
            // C11 6.2.1p4: identifiers with inner scope shadow outer ones.
            // A variable in scope (locals, enclosing nested-function frames,
            // then globals) must win over a global enum constant of the same
            // name; consulting find_enum_const() first resolved e.g. the
            // global `enum filetype` enumerator `directory` (readline's
            // colors.h) in place of a local `static DIR *directory`, folding
            // the variable to the enum's numeric value (3) and then crashing
            // codegen with "Invalid register -1" when the non-lvalue was
            // assigned to. Variables take precedence; the special keywords
            // below stay as-is (they can never name a variable).
            LVar *var = find_var(tok);
            if (var) {
                // C11 6.7.4p3: a non-static, non-extern-inline (i.e. an
                // inline function that MAY be emitted as an external
                // definition somewhere else) may not reference a
                // modifiable object with internal linkage -- the object
                // might not be visible wherever that other definition
                // lives. GCC diagnoses this unconditionally (confirmed:
                // "'sv' is static but used in inline function 'j' which
                // is not static", present even with no flags at all).
                // current_fn_is_inline already excludes `extern inline`
                // (see its assignment below) -- extern inline genuinely
                // IS the one external definition, right here, where the
                // static object is visible, so gcc never warns there
                // (rpmalloc's malloc.c: every rpvalloc/rppvalloc/etc. is
                // "extern inline").
                if (current_fn_is_inline && var->is_static && !var->is_local &&
                    var->ty && !(var->ty->qual & QUAL_CONST) &&
                    find_global_name(tok->name) == var)
                    warn_tok(tok, "'%s' is static but used in inline function '%s' which is not static",
                             tok->name, parser_current_fn ? parser_current_fn : "?");
                node = new_var_node(var, tok);
                node->chain_depth = last_find_var_chain_depth;
                tok = tok->next;
            } else {
                EnumConst *ec = find_enum_const(tok);
                if (ec) {
                    node = new_num(ec->val, tok);
                    if (ec->ty)
                        node->ty = ec->ty; // C23 enumerator type (enum/uint/llong...)
                    tok = tok->next;
                } else if (equalc(tok, "NULL")) {
                    node = new_num(0, tok);
                    tok = tok->next;
                } else if (equalc(tok, "nullptr")) {
                    node = new_num(0, tok);
                    node->ty = ty_nullptr_t;
                    tok = tok->next;
                } else if (tok->len > 10 && !memcmp(tok->ptr, "__builtin_", 10) &&
                           (node = declare_builtin_on_demand(tok))) {
                    tok = tok->next;
                    node = new_var_node(node->var, tok);
                    node->ty = node->var->ty;
                } else if (equalc(tok, "true")) {
                    // C23 keyword: bool-typed constant 1
                    node = new_num(1, tok);
                    node->ty = ty_bool;
                    tok = tok->next;
                } else if (equalc(tok, "false")) {
                    // C23 keyword: bool-typed constant 0
                    node = new_num(0, tok);
                    node->ty = ty_bool;
                    tok = tok->next;
                } else {
                    error_tok(tok, "undeclared variable");
                }
            }
        }
    } else if (equalc(tok, "&&") && tok->next && tok->next->kind == TK_IDENT) {
        // GCC label address: &&label
        node = new_node(ND_LABEL_VAL, tok);
        node->label_name = tok->next->name;
        node->ty = pointer_to(ty_void);
        {
            int owner_depth;
            node->funcname = find_label_scope_owner(node->label_name, &owner_depth);
        }
        tok = tok->next->next;
    } else if (tok->kind == TK_NUM) {
        node = new_num(tok->val, tok);
        // Prefixed character constants carry their own type:
        // u8'' -> unsigned char (char8_t), u'' -> char16_t, U'' -> char32_t.
        // Plain and L'' constants keep type int.
        switch (tok->string_literal_prefix) {
        case '8': node->ty = ty_uchar; break;
        case 'u': node->ty = ty_ushort; break;
        case 'U': node->ty = ty_uint; break;
        default: break;
        }
        tok = tok->next;
    } else if (tok->kind == TK_FNUM) {
        if (tok->val & 4) {
            // Imaginary literal: create complex with real=0, imag=val
            if (tok->val & 8) {
                // Integer imaginary (e.g., 200i): _Complex int
                Type *cty = complex_type(ty_int);
                Node *zero = new_num(0, tok);
                zero->ty = ty_int;
                Node *imag = new_num((int64_t)tok->fval, tok);
                imag->ty = ty_int;
                node = new_complex_val(zero, imag, cty, tok);
            } else {
                int fkind = tok->val & 3;
                Type *base = fkind == 1 ? ty_float : fkind == 2 ? ty_ldouble
                                                                : ty_double;
                Type *cty = complex_type(base);
                Node *zero = new_fnum(0.0, tok);
                zero->ty = base;
                Node *imag = new_fnum(tok->fval, tok);
                imag->ty = base;
                node = new_complex_val(zero, imag, cty, tok);
            }
            tok = tok->next;
        } else {
            Node *dec = new_decimal(tok);
            if (dec) {
                node = dec;
            } else {
                node = new_fnum(tok->fval, tok);
                if (tok->val == 1)
                    node->ty = ty_float;
            }
            tok = tok->next;
        }
    } else if (tok->kind == TK_STR) {
        node = new_node(ND_STR, tok);
        node->str = tok->str;
        // Set the type based on the string literal prefix
        // Use array type so sizeof works correctly; decays to pointer where needed
        switch (tok->string_literal_prefix) {
        case 0: // Regular string
            node->ty = array_of(ty_char, tok->len + 1);
            break;
        case 'L': { // Wide string
            Type *wchar_ty =
#ifdef _WIN32
                ty_ushort;
#else
                ty_uint;
#endif
            // A string literal's expression type is an array of N+1
            // elements (the trailing NUL), matching how the plain-char
            // case above always worked - it decays to a pointer only in
            // rvalue contexts (handled generically wherever TY_ARRAY
            // decays), never pre-decayed here. Getting this wrong breaks
            // typeof()/sizeof()/__builtin_types_compatible_p, which must
            // see the un-decayed array (e.g. typeof(u8"abc") ==
            // unsigned char[4], not unsigned char*). Element count is the
            // decoded codepoint count, not the raw UTF-8 byte length.
            int n = 0;
            for (char *p = tok->str, *end = p + tok->len; p < end; n++) {
                char *next_p;
                decode_utf8(&next_p, p);
                p = next_p;
            }
            node->ty = array_of(wchar_ty, n + 1);
        } break;
        case 'u': { // char16_t string
            Type *char16_t_type = typedef_find_name("char16_t");
            if (!char16_t_type) char16_t_type = ty_ushort;
            int n = 0;
            for (char *p = tok->str, *end = p + tok->len; p < end; n++) {
                char *next_p;
                decode_utf8(&next_p, p);
                p = next_p;
            }
            node->ty = array_of(char16_t_type, n + 1);
        } break;
        case '8': // u8 string => char8_t (unsigned char) in C23, char before
            if (opt_std_version && strcmp(opt_std_version, "202311L") >= 0)
                node->ty = array_of(ty_uchar, tok->len + 1);
            else
                node->ty = array_of(ty_char, tok->len + 1);
            break;
        case 'U': { // char32_t string
            Type *char32_t_type = typedef_find_name("char32_t");
            if (!char32_t_type) char32_t_type = ty_uint;
            int n = 0;
            for (char *p = tok->str, *end = p + tok->len; p < end; n++) {
                char *next_p;
                decode_utf8(&next_p, p);
                p = next_p;
            }
            node->ty = array_of(char32_t_type, n + 1);
        } break;
        default: // Fallback to regular string
            node->ty = array_of(ty_char, tok->len + 1);
            break;
        }
        StrLit *s = new_str_lit(tok->str, tok->len, tok->string_literal_prefix, node->ty->base->size);
        node->str_id = s->id;
        tok = tok->next;
    } else {
        error_tok(tok, "expected an expression");
    }

    return apply_postfix_ops(node, rest, tok);
}

// Apply postfix operators (call, subscript, member access, post-inc/dec) to
// an already-parsed primary expression. Extracted from primary()'s own
// trailing chain so other constructs whose result is itself a primary
// expression — e.g. unary()'s __builtin_choose_expr — can run the very same
// chain instead of returning bare and leaving any "->"/"."/"[...]" that
// follows unparsed. Real-world need: drivers/gpu/drm/i915/gt/intel_gtt.h's
// px_pt()/px_used() macros expand to "&__builtin_choose_expr(...)->used" —
// a postfix "->" directly on the choose_expr's result.
static Node *apply_postfix_ops(Node *node, Token **rest, Token *tok) {
    check_type(node);
    while (true) {
        if (equalc(tok, "(")) {
            Node *call = new_node(ND_FUNCALL, tok);
            call->lhs = node;
            tok = tok->next;
            Node head = {};
            Node *cur = &head;
            while (!equalc(tok, ")")) {
                if (cur != &head)
                    tok = skip(tok, ",");
                cur = cur->next = assign(&tok, tok);
            }
            call->args = head.next;
            tok = skip(tok, ")");
            cast_funcall_args(call);
            node = call;
            check_type(node);
            continue;
        }
        if (equalc(tok, "[")) {
            Token *start = tok;
            Node *idx = expr(&tok, tok->next);
            tok = skip(tok, "]");
            if (node->ty && node->ty->is_vector) {
                // Vector subscript v[i]: vectors are TY_STRUCT (not arrays), so
                // synthesize element access via the vector's address:
                //   v[i]  =>  *((elem *)&v + i)
                Node *addr = new_unary(ND_ADDR, node, start);
                addr->ty = pointer_to(node->ty->base);
                node = new_unary(ND_DEREF, new_binary(ND_ADD, addr, idx, start), start);
            } else {
                node = new_unary(ND_DEREF, new_binary(ND_ADD, node, idx, start), start);
            }
            check_type(node);
            continue;
        }
        if (equalc(tok, ".")) {
            tok = tok->next;
            check_type(node);
            Member *mem = find_member(node->ty, tok);
            if (!mem)
                error_tok(tok, "no such member");
            Node *mem_node = new_unary(ND_MEMBER, node, tok);
            mem_node->member = mem;
            mem_node->ty = member_access_type(node->ty, mem);
            node = mem_node;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "->")) {
            tok = tok->next;
            check_type(node);
            if ((node->ty->kind != TY_PTR && node->ty->kind != TY_ARRAY && node->ty->kind != TY_VLA) ||
                (node->ty->base->kind != TY_STRUCT && node->ty->base->kind != TY_UNION))
                error_tok(tok, "not a pointer to struct or union");
            node = new_unary(ND_DEREF, node, tok);
            check_type(node);
            Member *mem = find_member(node->ty, tok);
            if (!mem)
                error_tok(tok, "no such member");
            Node *mem_node = new_unary(ND_MEMBER, node, tok);
            mem_node->member = mem;
            mem_node->ty = member_access_type(node->ty, mem);
            node = mem_node;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "++")) {
            Node *vla = vla_ptr_incdec(node, true, true, tok);
            node = vla ? vla : new_unary(ND_POST_INC, node, tok);
            tok = tok->next;
            check_type(node);
            continue;
        }
        if (equalc(tok, "--")) {
            Node *vla = vla_ptr_incdec(node, false, true, tok);
            node = vla ? vla : new_unary(ND_POST_DEC, node, tok);
            tok = tok->next;
            check_type(node);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

static int parse_memory_order(Token **rest) {
    Token *tok = *rest;
    if (tok->kind == TK_NUM) {
        *rest = tok->next;
        return (int)tok->val;
    }
    EnumConst *ec = find_enum_const(tok);
    if (ec) {
        *rest = tok->next;
        return (int)ec->val;
    }
    Node *node = assign(rest, tok);
    check_type(node);
    if (node->kind == ND_NUM)
        return (int)node->val;
    return MEMORDER_SEQ_CST;
}

// Compute a Node* for the byte size of a type (may be runtime for VLA).
static Node *type_size_node(Type *ty, Token *tok) {
    if (ty->kind == TY_VLA) {
        Node *len = ty->vla_len_expr ? ty->vla_len_expr : new_num(ty->array_len, tok);
        Node *base = type_size_node(ty->base, tok);
        Node *result = new_binary(ND_MUL, len, base, tok);
        check_type(result);
        return result;
    }
    Node *n = new_num(ty->size, tok);
    n->ty = size_t_type(); // sizeof/byte-size is always size_t (unsigned) -- new_num()'s
    // suffix-sniffing reads tok's raw text looking for u/l, but
    // `tok` here is never actually the numeric-literal token this
    // value came from (it's whatever token followed the sizeof
    // expression), so that heuristic can't determine the right
    // type and must not be trusted for this call site.
    return n;
}

// ---- Vector (__attribute__((vector_size))) element-wise lowering ----------
// gen_vector() in codegen only covers 16-byte two-vector ops that map to a
// packed SSE instruction. Everything else — integer mul/div/mod, shifts,
// scalar<->vector broadcast, integer compares, non-16-byte vectors — is
// lowered here at parse time into per-lane scalar ops:
//   a OP b  =>  (tA = a, tB = b, tR.__v0 = tA.__v0 OP tB.__v0, ..., tR)
// so the existing scalar codegen does the work on every target.

// Lane accessor: v.__v<i>
static Node *vec_lane(LVar *var, Type *vt, int i, Token *tok) {
    Member *mem = vt->members;
    for (int j = 0; j < i && mem; j++)
        mem = mem->next;
    Node *m = new_unary(ND_MEMBER, new_var_node(var, tok), tok);
    m->member = mem;
    m->ty = mem->ty;
    return m;
}

// Bind an operand to a fresh temp so it is evaluated exactly once.
// Appends the binding assignment to *chain and returns the temp.
static LVar *vec_bind(Node **chain, Node *operand, Type *ty, Token *tok) {
    LVar *v = new_var("", ty, true);
    Node *as = new_binary(ND_ASSIGN, new_var_node(v, tok), operand, tok);
    *chain = *chain ? new_binary(ND_COMMA, *chain, as, tok) : as;
    return v;
}

// Runtime lane access v[idx]: *((elem *)&v + idx)
static Node *vec_gather(LVar *var, Type *vt, Node *idx, Token *tok) {
    Node *addr = new_unary(ND_ADDR, new_var_node(var, tok), tok);
    addr->ty = pointer_to(vt->base);
    return new_unary(ND_DEREF, new_binary(ND_ADD, addr, idx, tok), tok);
}

static Node *vector_lower(Node *node) {
    NodeKind k = node->kind;
    bool un = (k == ND_NEG || k == ND_BITNOT);
    switch (k) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_BITAND:
    case ND_BITOR:
    case ND_BITXOR:
    case ND_SHL:
    case ND_SHR:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_NEG:
    case ND_BITNOT:
        break;
    default:
        return node;
    }
    check_type(node->lhs);
    if (!un && node->rhs)
        check_type(node->rhs);
    Type *lt = node->lhs ? node->lhs->ty : NULL;
    Type *rt = (!un && node->rhs) ? node->rhs->ty : NULL;
    // C23: nullptr_t is neither an integer nor a pointer type, so it is
    // never valid in ordered comparisons or arithmetic; == and != accept
    // it only against a pointer, another nullptr_t value, or a null
    // pointer constant (C23 6.5.9/6.5.10). Real GCC hard-errors every
    // one of these -- postgres's snprintf.c `strerror_r` misdeclaration
    // was masked by rcc silently accepting `switch (ptr_expr)`, a
    // sibling gap in the same "reject invalid nullptr_t/non-integer
    // uses" family; this is the arithmetic/comparison half.
    if (lt && rt && (lt->kind == TY_NULLPTR_T || rt->kind == TY_NULLPTR_T)) {
        if (k == ND_LT || k == ND_LE) {
            error_tok(node->tok, "ordered comparison of nullptr_t");
        } else if (k == ND_EQ || k == ND_NE) {
            Node *other = (lt->kind == TY_NULLPTR_T) ? node->rhs : node->lhs;
            if (other->ty->kind != TY_PTR && !is_null_value_or_nullptr(other))
                error_tok(node->tok, "invalid operands to binary %s", k == ND_EQ ? "==" : "!=");
        } else {
            error_tok(node->tok, "invalid operands to binary expression");
        }
    }
    bool lv = lt && lt->is_vector;
    bool rv = rt && rt->is_vector;
    if (!lv && !rv)
        return node;
    Type *vt = lv ? lt : rt;
    Type *elem = vt->base;
    int n = (int)(vt->size / elem->size);
    bool cmp = (k == ND_EQ || k == ND_NE || k == ND_LT || k == ND_LE);
    bool bitop = (k == ND_BITAND || k == ND_BITOR || k == ND_BITXOR || k == ND_BITNOT);
    bool arith = (k == ND_ADD || k == ND_SUB || k == ND_MUL || k == ND_DIV);
    // Float-vector arithmetic, compares and bitwise ops stay on the packed
    // SSE/NEON path (addps/mulps/cmpps/andps/...). They operate on the raw
    // lane values/bits, which per-lane scalar floating-point ops cannot express
    // correctly (compare masks are all-ones/zero, not scalar 1/0).
    if ((cmp || bitop || arith) && is_flonum(elem)) {
        node->ty = vt;
        if (!un && !lv) {
            Node *cast = new_unary(ND_CAST, node->lhs, node->tok);
            cast->ty = elem;
            node->lhs = cast;
        }
        if (!un && !rv) {
            Node *cast = new_unary(ND_CAST, node->rhs, node->tok);
            cast->ty = elem;
            node->rhs = cast;
        }
        return node;
    }
    Token *tok = node->tok;
    Node *chain = NULL;
    // Bind operands; a scalar operand is converted to the element type once
    // (GCC broadcast semantics). Shift counts keep integer type instead.
    LVar *ta, *tb = NULL;
    if (lv) {
        ta = vec_bind(&chain, node->lhs, lt, tok);
    } else {
        Node *cast = new_unary(ND_CAST, node->lhs, tok);
        cast->ty = elem;
        ta = vec_bind(&chain, cast, elem, tok);
    }
    if (!un) {
        if (rv) {
            tb = vec_bind(&chain, node->rhs, rt, tok);
        } else {
            Type *bt = (k == ND_SHL || k == ND_SHR) ? ty_int : elem;
            Node *cast = new_unary(ND_CAST, node->rhs, tok);
            cast->ty = bt;
            tb = vec_bind(&chain, cast, bt, tok);
        }
    }
    LVar *tr = new_var("", vt, true);
    for (int i = 0; i < n; i++) {
        Node *a = lv ? vec_lane(ta, lt, i, tok) : new_var_node(ta, tok);
        Node *val;
        if (un) {
            val = new_unary(k, a, tok);
        } else {
            Node *b = rv ? vec_lane(tb, rt, i, tok) : new_var_node(tb, tok);
            val = new_binary(k, a, b, tok);
        }
        if (cmp) {
            // GCC vector compare: lane = (a OP b) ? -1 : 0 in the lane width
            val = new_unary(ND_NEG, val, tok);
            // Type the child before presetting the cast type: add_type
            // early-returns on typed nodes without descending into children.
            check_type(val);
            Node *cast = new_unary(ND_CAST, val, tok);
            cast->ty = elem;
            val = cast;
        }
        Node *st = new_binary(ND_ASSIGN, vec_lane(tr, vt, i, tok), val, tok);
        chain = new_binary(ND_COMMA, chain, st, tok);
    }
    chain = new_binary(ND_COMMA, chain, new_var_node(tr, tok), tok);
    check_type(chain);
    return chain;
}

// Forward-declared: defined below, used by assign_nested_struct_init's
// nested array-member branch above it.
static Node *synth_struct_elem_literal(Type *elem_ty, Token **rest, Token *tok,
                                       Token *start, int *anon_count);

// Assign a brace-enclosed initializer into the struct/union member `mem` of
// lvalue `base` (i.e. `base.mem = { ... }`), appending ND_ASSIGN nodes onto
// `result`. Recurses when a sub-member is itself a struct/union with its own
// brace-enclosed value, so arbitrarily deep wrapper structs — very common in
// the kernel (atomic_t -> arch_spinlock_t -> raw_spinlock_t -> spinlock_t,
// e.g. `{ { .val = { 0 } } }`) — parse instead of only one level deep.
static Node *assign_nested_struct_init(Node *result, Node *base, Member *mem,
                                       Token **rest, Token *tok, Token *start, int *anon_count) {
    tok = skip(tok, "{");
    Node *member_access = new_node(ND_MEMBER, start);
    member_access->lhs = base;
    member_access->member = mem;
    member_access->ty = mem->ty;
    Member *sub = mem->ty->members;
    while (!equalc(tok, "}")) {
        Member *target = NULL;
        Node *target_base = member_access;
        if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
            // Walk a chain of .name1.name2...nameN designators — needed
            // for a macro-expanded multi-level field access, e.g. linux's
            // icmp6_router expanding to ".icmp6_dataun.u_nd_advt.router"
            // (net/ipv6/ndisc.c's "*msg = (struct nd_msg){ .icmph = {
            // .icmp6_router = router, ... } }"). All but the last level
            // fold into an accumulated ND_MEMBER chain (target_base); the
            // last becomes `target`, resolved against that accumulated
            // base exactly like a single-level designator already was.
            Node *chain_lhs = member_access;
            Type *chain_ty = mem->ty;
            Member *first_dm = NULL, *dm = NULL;
            bool chain_ok = true;
            for (;;) {
                char *name = tok->next->name;
                dm = find_member_by_name(chain_ty, name);
                if (!dm) {
                    chain_ok = false;
                    break;
                }
                tok = tok->next->next;
                if (!first_dm) first_dm = dm;
                // No further ".name" — this level is the final target.
                if (!(equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT))
                    break;
                Node *mem_node = new_unary(ND_MEMBER, chain_lhs, tok);
                mem_node->member = dm;
                check_type(mem_node);
                chain_lhs = mem_node;
                chain_ty = dm->ty;
            }
            if (chain_ok && dm) {
                target_base = chain_lhs;
                // A combined `.member[idx] = val` designator (C99 6.7.8p17)
                // leaves `[idx]` unconsumed here for an array-typed member —
                // the array-index branch below parses `[idx] = val`
                // (including the `=`) itself.
                if (!(dm->ty->kind == TY_ARRAY && equalc(tok, "[")))
                    tok = skip(tok, "=");
                target = dm;
                sub = first_dm->next;
            } else {
                tok = skip_initializer(tok);
            }
        } else if (sub) {
            target = sub;
            sub = sub->next;
        } else {
            tok = skip_initializer(tok);
        }
        if (target) {
            if (target->ty->kind == TY_ARRAY && equalc(tok, "[")) {
                // Designated array element(s) directly on a member: `[N] =
                // val` or `[N ... M] = val` (C99 6.7.8p17), e.g. the
                // property_entry union's `.u32_data[0] = val`. Mirrors the
                // array-designator loop used for top-level array members.
                Node *inner_access = new_node(ND_MEMBER, start);
                inner_access->lhs = target_base;
                inner_access->member = target;
                inner_access->ty = target->ty;
                int len = array_len(target->ty);
                tok = skip(tok, "[");
                Node *idx_lo = assign(&tok, tok);
                long long sv = 0;
                eval_const_expr(idx_lo, &sv);
                long long ev = sv;
                if (equalc(tok, "...")) {
                    tok = tok->next;
                    Node *idx_hi = assign(&tok, tok);
                    eval_const_expr(idx_hi, &ev);
                }
                tok = skip(tok, "]");
                // The indexed element may itself carry a chained member
                // designator: ".attrs[0].format = val" (C99 6.7.8p17's
                // designator chain continuing through an array-index
                // step) — sokol's sg_pipeline_desc initializers nest
                // exactly this shape. Walk it to a leaf member instead
                // of demanding "=" right after "]".
                Member *elem_mem = NULL;
                Type *elem_chain_ty = target->ty->base;
                int elem_chain_offset = 0;
                while (elem_chain_ty &&
                       (elem_chain_ty->kind == TY_STRUCT || elem_chain_ty->kind == TY_UNION) &&
                       equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                    Member *sm = find_member_by_name(elem_chain_ty, tok->next->name);
                    if (!sm) break;
                    elem_mem = sm;
                    elem_chain_offset += sm->offset;
                    elem_chain_ty = sm->ty;
                    tok = tok->next->next;
                }
                tok = skip(tok, "=");
                Token *val_start = tok;
                for (long long i = sv; i <= ev; i++) {
                    tok = val_start;
                    Node *offset = new_num(i, start);
                    Node *elem_ptr = new_binary(ND_ADD, inner_access, offset, start);
                    Node *elem_lhs = new_unary(ND_DEREF, elem_ptr, start);
                    if (elem_mem) {
                        check_type(elem_lhs);
                        Member *leaf = elem_mem;
                        if (elem_chain_offset != leaf->offset) {
                            Member *syn = arena_alloc(sizeof(Member));
                            *syn = *leaf;
                            syn->offset = elem_chain_offset;
                            leaf = syn;
                        }
                        Node *sub_access = new_node(ND_MEMBER, start);
                        sub_access->lhs = elem_lhs;
                        sub_access->member = leaf;
                        sub_access->ty = leaf->ty;
                        elem_lhs = sub_access;
                    }
                    Node *val = assign(&tok, tok);
                    check_type(val);
                    if (len == 0 || i < len) {
                        Node *asgn = new_binary(ND_ASSIGN, elem_lhs, val, start);
                        check_type(asgn);
                        result = new_binary(ND_COMMA, result, asgn, start);
                    }
                }
            } else if ((target->ty->kind == TY_STRUCT || target->ty->kind == TY_UNION) && equalc(tok, "{")) {
                result = assign_nested_struct_init(result, target_base, target, &tok, tok, start, anon_count);
            } else if (target->ty->kind == TY_ARRAY && equalc(tok, "{")) {
                // Nested array member given as its own brace-enclosed
                // initializer, one level deeper than the top-level
                // array-member case — positional and/or [N]/[N...M]=val
                // designated elements, e.g. kefir's
                // ".parameters = {.refs = {condition_ref, ref1, ref2}}".
                Node *inner_access = new_node(ND_MEMBER, start);
                inner_access->lhs = target_base;
                inner_access->member = target;
                inner_access->ty = target->ty;
                tok = tok->next; // skip "{"
                int len = array_len(target->ty);
                int aidx = 0;
                while (!equalc(tok, "}")) {
                    int sidx = aidx, eidx = aidx;
                    if (equalc(tok, "[")) {
                        tok = tok->next;
                        Node *n = assign(&tok, tok);
                        long long sv = 0;
                        eval_const_expr(n, &sv);
                        sidx = (int)sv;
                        eidx = sidx;
                        if (equalc(tok, "...")) {
                            tok = tok->next;
                            Node *n2 = assign(&tok, tok);
                            long long ev = sidx;
                            eval_const_expr(n2, &ev);
                            eidx = (int)ev;
                        }
                        tok = skip(tok, "]");
                        tok = skip(tok, "=");
                        aidx = sidx;
                    }
                    Token *val_start = tok;
                    for (int i = sidx; i <= eidx; i++) {
                        if (len == 0 || i < len) {
                            Node *offset = new_num(i, start);
                            Node *elem_ptr = new_binary(ND_ADD, inner_access, offset, start);
                            Node *elem_lhs = new_unary(ND_DEREF, elem_ptr, start);
                            check_type(elem_lhs);
                            tok = val_start;
                            Node *val = (equalc(tok, "{") && target->ty->base &&
                                         (target->ty->base->kind == TY_STRUCT || target->ty->base->kind == TY_UNION))
                                ? synth_struct_elem_literal(target->ty->base, &tok, tok, start, anon_count)
                                : assign(&tok, tok);
                            check_type(val);
                            Node *asgn = new_binary(ND_ASSIGN, elem_lhs, val, start);
                            check_type(asgn);
                            result = new_binary(ND_COMMA, result, asgn, start);
                        }
                    }
                    aidx = eidx + 1;
                    if (equalc(tok, ",")) {
                        tok = tok->next;
                        if (equalc(tok, "}")) break;
                        continue;
                    }
                    break;
                }
                tok = skip(tok, "}");
            } else {
                // A lone extra brace layer around a non-aggregate value is a
                // legal GNU/C11 redundant-brace idiom, e.g. `{ { 0 } }`.
                bool extra_brace = equalc(tok, "{");
                if (extra_brace) tok = tok->next;
                Node *inner_access = new_node(ND_MEMBER, start);
                inner_access->lhs = target_base;
                inner_access->member = target;
                inner_access->ty = target->ty;
                Node *val = (extra_brace && equalc(tok, "}")) ? new_num(0, start) : assign(&tok, tok);
                check_type(val);
                if (extra_brace) {
                    if (equalc(tok, ",")) tok = tok->next;
                    tok = skip(tok, "}");
                }
                Node *asgn = new_binary(ND_ASSIGN, inner_access, val, start);
                check_type(asgn);
                result = new_binary(ND_COMMA, result, asgn, start);
            }
        }
        if (equalc(tok, ",")) {
            tok = tok->next;
            if (equalc(tok, "}")) break;
            continue;
        }
        break;
    }
    tok = skip(tok, "}");
    *rest = tok;
    return result;
}

// Synthesize a nested (ElemType){...} compound-literal value for a
// braced struct/union initializer used as an ARRAY ELEMENT's value —
// e.g. a designated array index `[N] = { .field = val, ... }` where the
// array's element type is itself a struct/union. assign() alone can't
// parse a bare "{...}" as an expression, so any caller handing it a
// braced value must go through this instead. Shared by the top-level
// array-compound-literal path and a struct compound literal's own
// array-typed *member* (below) — the latter used to call assign()
// unconditionally on a designated array index's value, so a nested
// struct/union element (arch/x86/mm/init.c's execmem_info: `.ranges = {
// [EXECMEM_MODULE_TEXT] = { .flags = ..., .start = ..., ... }, ... }`)
// hit "expected an expression" on the inner brace.
static Node *synth_struct_elem_literal(Type *elem_ty, Token **rest, Token *tok,
                                       Token *start, int *anon_count) {
    Token *fake_start = tok; // tok == '{'
    tok = tok->next; // skip '{'
    char *ename = format(".Lanon.%d", (*anon_count)++);
    LVar *evar = new_var(ename, elem_ty, true);
    Node *ezinit = new_node(ND_ZERO_INIT, start);
    ezinit->lhs = new_var_node(evar, start);
    Node *eres = new_binary(ND_COMMA, ezinit, new_var_node(evar, start), start);
    Member *emem = elem_ty->members;
    while (!equalc(tok, "}")) {
        if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
            // Walk a chain of .name1.name2...nameN designators — needed
            // for a macro-expanded multi-level field access, e.g.
            // drivers/gpu/drm/i915/display/intel_display_power_map.c's
            // I915_PW() macro: ".hsw.idx = HSW_PW_CTL_IDX_GLOBAL" (hsw is
            // a named member of an anonymous union; idx is a member of
            // hsw's own inner struct type). All but the last level fold
            // into an accumulated ND_MEMBER chain (base); the last is the
            // actual assignment target, exactly like a single-level
            // designator already was.
            Type *cur_ty = elem_ty;
            Node *base = new_var_node(evar, start);
            Member *first = NULL, *m = NULL;
            for (;;) {
                if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                    char *mname = tok->next->name;
                    m = find_member_by_name(cur_ty, mname);
                    if (!m) break;
                    if (!first) first = m;
                    tok = tok->next->next;
                    Node *ma = new_unary(ND_MEMBER, base, fake_start);
                    ma->member = m;
                    check_type(ma);
                    base = ma;
                    cur_ty = m->ty;
                    continue;
                }
                if (equalc(tok, "[") && cur_ty->kind == TY_ARRAY) {
                    // Array-index step in the designator chain
                    // (".arr[0] = ...", C99 6.7.8p17): sokol's sg_shader_desc
                    // initializers nest exactly this shape
                    // (".uniform_blocks[0] = { ... .glsl_uniforms[0] = ... }").
                    // The chain walk below used to stop at the member,
                    // leaving "[0]" for skip(tok, "=") to stumble over
                    // ("expected specific operator").
                    tok = tok->next;
                    Node *idx = assign(&tok, tok);
                    tok = skip(tok, "]");
                    check_type(idx);
                    Node *sub = new_unary(ND_DEREF,
                                          new_binary(ND_ADD, base, idx, fake_start), fake_start);
                    check_type(sub);
                    base = sub;
                    cur_ty = cur_ty->base;
                    continue;
                }
                break;
            }
            if (m) {
                tok = skip(tok, "=");
                // A nested struct/union member's own value may itself be a
                // braced sub-initializer (e.g. i915_pmu.c's device_attribute
                // "{ .attr = {.name = _name, .mode = _mode}, ... }") — bare
                // assign() can't parse a leading "{", so recurse the same
                // way the outer element dispatch does. Dispatch on the
                // chain's FINAL type (cur_ty), not m->ty: after an
                // "[idx]" step the leaf is the array ELEMENT type, e.g.
                // ".glsl_uniforms[0] = { ... }" where glsl_uniforms is an
                // array of structs.
                if (equalc(tok, "{") && cur_ty->kind == TY_ARRAY && cur_ty->base) {
                    // Braced array member value (".glsl_uniforms = { [0] =
                    // { ... }, [1] = { ... } }") — append per-element
                    // assigns directly to the result chain (an array
                    // cannot be the RHS of a single assignment). Mirrors
                    // the top-level array-member loop; the element value
                    // recurses through this same synthesizer when it is
                    // itself a braced struct/union init.
                    tok = tok->next; // skip "{"
                    int aidx = 0;
                    while (!equalc(tok, "}")) {
                        int sidx = aidx, eidx = aidx;
                        if (equalc(tok, "[")) {
                            tok = tok->next;
                            Node *n = assign(&tok, tok);
                            long long sv = 0;
                            eval_const_expr(n, &sv);
                            sidx = (int)sv;
                            eidx = sidx;
                            if (equalc(tok, "...")) {
                                tok = tok->next;
                                Node *n2 = assign(&tok, tok);
                                long long ev = sidx;
                                eval_const_expr(n2, &ev);
                                eidx = (int)ev;
                            }
                            tok = skip(tok, "]");
                            tok = skip(tok, "=");
                            aidx = sidx;
                        }
                        Token *vstart = tok;
                        for (int i = sidx; i <= eidx; i++) {
                            tok = vstart;
                            Node *off = new_num(i, start);
                            Node *ep = new_binary(ND_ADD, base, off, start);
                            Node *el = new_unary(ND_DEREF, ep, start);
                            check_type(el);
                            Node *ev = (equalc(tok, "{") && cur_ty->base &&
                                        (cur_ty->base->kind == TY_STRUCT || cur_ty->base->kind == TY_UNION))
                                ? synth_struct_elem_literal(cur_ty->base, &tok, tok, start, anon_count)
                                : assign(&tok, tok);
                            check_type(ev);
                            Node *ea = new_binary(ND_ASSIGN, el, ev, start);
                            check_type(ea);
                            eres = new_binary(ND_COMMA, eres, ea, start);
                        }
                        aidx = eidx + 1;
                        if (equalc(tok, ",")) {
                            tok = tok->next;
                            if (equalc(tok, "}")) break;
                            continue;
                        }
                        break;
                    }
                    tok = skip(tok, "}");
                } else {
                    Node *v2 = (equalc(tok, "{") && (cur_ty->kind == TY_STRUCT || cur_ty->kind == TY_UNION))
                        ? synth_struct_elem_literal(cur_ty, &tok, tok, start, anon_count)
                        : assign(&tok, tok);
                    check_type(v2);
                    Node *a2 = new_binary(ND_ASSIGN, base, v2, start);
                    check_type(a2);
                    eres = new_binary(ND_COMMA, eres, a2, start);
                }
                emem = first->next;
            } else {
                assign(&tok, tok);
            }
        } else if (emem) {
            Node *ma = new_unary(ND_MEMBER, new_var_node(evar, start), fake_start);
            ma->member = emem;
            check_type(ma);
            Node *v2 = (equalc(tok, "{") && (emem->ty->kind == TY_STRUCT || emem->ty->kind == TY_UNION))
                ? synth_struct_elem_literal(emem->ty, &tok, tok, start, anon_count)
                : assign(&tok, tok);
            check_type(v2);
            Node *a2 = new_binary(ND_ASSIGN, ma, v2, start);
            check_type(a2);
            eres = new_binary(ND_COMMA, eres, a2, start);
            emem = emem->next;
        } else {
            assign(&tok, tok);
        }
        if (equalc(tok, ",")) {
            tok = tok->next;
            if (equalc(tok, "}")) break;
            continue;
        }
        break;
    }
    tok = skip(tok, "}");
    Node *efinal = new_var_node(evar, start);
    Node *val = new_binary(ND_COMMA, eres, efinal, start);
    check_type(val);
    *rest = tok;
    return val;
}

// GCC inlines the libatomic helper names __atomic_<op>_<N> (N = 1/2/4/8/16)
// as builtins, semantically identical to the __atomic_<op>_n / generic
// __atomic_<op> forms rcc already handles -- glib's own gatomic.h calls
// __atomic_load_4/8 and __atomic_store_4/8 directly. Recognize the _N
// suffix so those calls lower to the same inline ND_ATOMIC_* nodes instead
// of being emitted as unresolved libatomic symbols at link time.
static bool atomic_lib_helper(Token *tok, const char *op) {
    if (!tok || tok->kind != TK_IDENT || !tok->ptr) return false;
    static const char *pfx = "__atomic_";
    size_t pl = strlen(pfx), ol = strlen(op);
    if (tok->len < (int)(pl + ol + 2) || tok->len > (int)(pl + ol + 3)) return false;
    if (memcmp(tok->ptr, pfx, pl) != 0) return false;
    if (memcmp(tok->ptr + pl, op, ol) != 0) return false;
    if (tok->ptr[pl + ol] != '_') return false;
    const char *sz = tok->ptr + pl + ol + 1;
    int n = tok->len - (int)(pl + ol + 1);
    if (n == 1)
        return sz[0] == '1' || sz[0] == '2' || sz[0] == '4' || sz[0] == '8';
    if (n == 2)
        return sz[0] == '1' && sz[1] == '6';
    return false;
}

static Node *unary(Token **rest, Token *tok) {
    // GCC __builtin_assoc_barrier(x): optimization barrier - evaluates to
    // x unchanged but blocks FP reassociation/constant folding across it.
    // rcc does not reassociate FP expressions, so the identity is exact;
    // it must also work for struct/union arguments (returned by value),
    // where treating the name as an unknown function broke (the call's
    // implicit-int result was address-taken for the struct return copy).
    if (equalc(tok, "__builtin_assoc_barrier")) {
        tok = skip(tok->next, "(");
        Node *arg = assign(&tok, tok);
        *rest = skip(tok, ")");
        check_type(arg);
        return arg;
    }
    if (equalc(tok, "__builtin_offsetof")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Type *ty = type_name(&tok, tok);
        tok = skip(tok, ",");

        int const_offset = 0;
        Node *rt_expr = NULL; // non-NULL once we need runtime computation

        while (true) {
            if (tok->kind != TK_IDENT)
                error_tok(tok, "expected member name");
            Member *mem = find_member(ty, tok);
            if (!mem)
                error_tok(tok, "no such member");
            const_offset += mem->offset;
            ty = mem->ty;
            tok = tok->next;

            while (equalc(tok, "[")) {
                tok = tok->next;
                if (ty->kind != TY_ARRAY && ty->kind != TY_VLA)
                    error_tok(tok, "unsupported offsetof designator");

                // Always parse the full subscript expression first — a
                // leading TK_NUM (e.g. the "1" in "1ul << bits") is not
                // necessarily the *whole* index; it may only be the start
                // of a larger constant (or non-constant) expression, which
                // a bare tok->kind == TK_NUM check on just the first token
                // wrongly assumed (leaving "<< bits]" unconsumed and
                // desyncing the parser). Decide constant vs. runtime only
                // after eval_const_expr() has actually evaluated it.
                Node *idx = assign(&tok, tok);
                tok = skip(tok, "]");
                check_type(idx);
                long long idx_val;
                if (!rt_expr && ty->kind == TY_ARRAY && eval_const_expr(idx, &idx_val)) {
                    // Constant subscript on constant-size array
                    const_offset += (int)(idx_val * ty->base->size);
                } else {
                    // Variable subscript or VLA: build runtime expression
                    Node *elem_sz = type_size_node(ty->base, tok);
                    Node *mul = new_binary(ND_MUL, idx, elem_sz, tok);
                    check_type(mul);
                    // Fold accumulated const_offset into rt_expr
                    Node *base_off = new_num(const_offset, tok);
                    check_type(base_off);
                    rt_expr = rt_expr
                        ? new_binary(ND_ADD, rt_expr, base_off, tok)
                        : base_off;
                    rt_expr = new_binary(ND_ADD, rt_expr, mul, tok);
                    check_type(rt_expr);
                    const_offset = 0;
                }
                ty = ty->base;
            }

            if (!equalc(tok, "."))
                break;
            tok = tok->next;
        }

        *rest = skip(tok, ")");
        // offsetof() returns size_t per C11 7.19p3 -- previously these
        // returned a plain new_num()/ND_ADD chain, which types as `int`
        // (matching the internal computation's own int-typed pieces, not
        // the standard-mandated size_t). Real code relies on this: e.g.
        // gnulib's test-stddef-h.c static_asserts
        // `sizeof (offsetof (struct d, e)) == sizeof (size_t)` and that
        // `offsetof (...) < -1` compares as unsigned (a small offset is
        // always less than (size_t)-1's huge value) -- both silently
        // failed with the narrower `int` type. size_t is 64-bit on every
        // rcc target; ty_ulong is only 64-bit on LP64 Linux/macOS and
        // 32-bit on Windows (LLP64), so use ty_ullong like
        // __builtin_object_size/__builtin_dynamic_object_size above.
        if (!rt_expr) {
            Node *n = new_num(const_offset, start);
            n->ty = ty_ullong;
            return n;
        }
        // Add any trailing const_offset to the runtime expression
        if (const_offset != 0) {
            Node *tail = new_num(const_offset, start);
            check_type(tail);
            rt_expr = new_binary(ND_ADD, rt_expr, tail, start);
            check_type(rt_expr);
        }
        if (rt_expr->ty != ty_ullong) {
            Node *cast = new_unary(ND_CAST, rt_expr, start);
            cast->ty = ty_ullong;
            rt_expr = cast;
        }
        return rt_expr;
    }
    // __builtin_object_size(ptr, type) — returns compile-time object size,
    // or (size_t)-1 (modes 0/1) / 0 (modes 2/3) when unknown
    if (equalc(tok, "__builtin_object_size")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        tok = skip(tok, ",");
        Node *mode_node = assign(&tok, tok);
        *rest = skip(tok, ")");
        long long mode = 0;
        eval_const_expr(mode_node, &mode);
        size_t sz = (mode >= 2) ? 0 : (size_t)-1;
        if (ptr) {
            Node *obj = ptr;
            while (obj->kind == ND_CAST && obj->lhs)
                obj = obj->lhs;
            if (obj->kind == ND_ADDR && obj->lhs)
                obj = obj->lhs;
            while (obj->kind == ND_CAST && obj->lhs)
                obj = obj->lhs;
            if (obj->kind == ND_LVAR && obj->var && obj->var->ty && obj->var->ty->size > 0) {
                Type *t = obj->var->ty;
                if (t->kind == TY_ARRAY || t->kind == TY_STRUCT || t->kind == TY_UNION)
                    sz = (size_t)t->size;
            } else if (obj->kind == ND_STR && obj->ty &&
                       obj->ty->kind == TY_ARRAY && obj->ty->size > 0) {
                sz = (size_t)obj->ty->size;
            } else if (obj->kind == ND_DEREF && obj->lhs) {
                // &arr[i]: remaining size of a known array object. Pointer
                // arithmetic is already scaled, so the non-array operand of
                // ND_ADD is the constant byte offset. Anything else stays
                // unknown: guessing (e.g. the element size) makes fortify
                // checks fail with false overflows.
                Node *padd = obj->lhs;
                Node *base = padd, *off_expr = NULL;
                if (padd->kind == ND_ADD) {
                    base = padd->lhs;
                    off_expr = padd->rhs;
                    if (base->kind != ND_LVAR) {
                        base = padd->rhs;
                        off_expr = padd->lhs;
                    }
                }
                long long off = 0;
                if (base->kind == ND_LVAR && base->var && base->var->ty &&
                    base->var->ty->kind == TY_ARRAY && base->var->ty->size > 0 &&
                    (!off_expr || eval_const_expr(off_expr, &off)) &&
                    off >= 0 && off <= base->var->ty->size)
                    sz = (size_t)(base->var->ty->size - off);
            }
        }
        Node *node = new_num((int64_t)sz, start);
        // size_t is 64-bit on every rcc target; ty_ulong is only 64-bit on
        // the LP64 Linux/macOS builds and 32-bit on Windows (LLP64).
        node->ty = ty_ullong;
        return node;
    }
    // __builtin_clear_padding(ptr) — zero all padding bytes in the pointed-to object.
    // Implemented as memset(ptr, 0, sizeof(*ptr)) for simplicity.
    if (equalc(tok, "__builtin_clear_padding")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        *rest = skip(tok, ")");
        if (ptr && ptr->ty && ptr->ty->kind == TY_PTR && ptr->ty->base && ptr->ty->base->size > 0) {
            Node *size = new_num(ptr->ty->base->size, start);
            size->ty = ty_ulong;
            Node *zero = new_num(0, start);
            zero->ty = ty_int;
            Node *fn = new_node(ND_FUNCALL, start);
            fn->funcname = str_intern("memset", 6);
            fn->args = ptr;
            ptr->next = zero;
            zero->next = size;
            fn->ty = pointer_to(ty_void);
            return fn;
        }
        return new_node(ND_NULL, start);
    }
    // __builtin_dynamic_object_size(ptr, type) — compile-time object size for
    // known stack/global objects, (size_t)-1 (modes 0/1) / 0 (modes 2/3) when
    // the pointed-to object is unknown.
    //
    // An earlier implementation emitted a RUNTIME read of the glibc malloc
    // chunk header (`*(size_t*)(ptr-8) & ~15 - 16`) for any pointer that
    // wasn't a known stack/global object, on the theory that _FORTIFY_SOURCE=3
    // heap checks could then use the real allocated size. That read is only
    // valid for a pointer actually pointing into a glibc-malloc'd chunk;
    // applied to a pointer to a STACK object that merely passed through a
    // function parameter (e.g. libsodium's `sodium_memzero(void *pnt, size_t
    // len)` -> `explicit_bzero(pnt, len)` with a stack array argument), it
    // read stack garbage, and the bogus "size" made glibc's fortify
    // `__explicit_bzero_chk` falsely abort with "*** buffer overflow
    // detected ***" on perfectly valid code. GCC's own
    // __builtin_dynamic_object_size returns -1 for any pointer whose
    // allocation it cannot trace (function parameters included); do the same.
    if (equalc(tok, "__builtin_dynamic_object_size")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        tok = skip(tok, ",");
        Node *mode_node = assign(&tok, tok);
        *rest = skip(tok, ")");
        long long mode = 0;
        eval_const_expr(mode_node, &mode);
        size_t sz = (mode >= 2) ? 0 : (size_t)-1;
        if (ptr) {
            Node *obj = ptr;
            while (obj->kind == ND_CAST && obj->lhs)
                obj = obj->lhs;
            if (obj->kind == ND_ADDR && obj->lhs)
                obj = obj->lhs;
            while (obj->kind == ND_CAST && obj->lhs)
                obj = obj->lhs;
            if (obj->kind == ND_LVAR && obj->var && obj->var->ty && obj->var->ty->size > 0) {
                Type *t = obj->var->ty;
                if (t->kind == TY_ARRAY || t->kind == TY_STRUCT || t->kind == TY_UNION)
                    sz = (size_t)t->size;
            }
        }
        Node *node = new_num((int64_t)sz, start);
        // size_t is 64-bit on every rcc target; ty_ulong is only 64-bit on
        // the LP64 Linux/macOS builds and 32-bit on Windows (LLP64), which
        // would truncate -1 to 0xFFFFFFFF and break size_t comparisons.
        node->ty = ty_ullong;
        return node;
    }
    // __builtin_complex(re, im) — construct a complex value from two
    // same-type real-floating arguments (GCC builtin, usable in constant
    // expressions and static initializers).
    if (equalc(tok, "__builtin_complex")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *re = assign(&tok, tok);
        tok = skip(tok, ",");
        Node *im = assign(&tok, tok);
        *rest = skip(tok, ")");
        check_type(re);
        check_type(im);
        Type *base = re->ty && is_flonum(re->ty) ? re->ty : ty_double;
        return new_complex_val(re, im, complex_type(base), start);
    }
    // __builtin_creal/crealf/creall(z) and __builtin_cimag/cimagf/cimagl(z) —
    // extract the real/imag component inline as a pure component load.
    // Never call libm's creal/cimag: rcc's internal complex layout and the
    // external ABI disagree for _Complex long double (SysV passes the
    // 32-byte value on the stack as x87 80-bit components, which rcc
    // neither pushes nor represents), so glibc's creall/cimagl returned
    // garbage and corrupted every mpc value that passed through them
    // (mpc_set_ldc/mpc_get_ldc produced @NaN@ components).
    if (equalc(tok, "__builtin_creal") || equalc(tok, "__builtin_crealf") ||
        equalc(tok, "__builtin_creall") || equalc(tok, "__builtin_cimag") ||
        equalc(tok, "__builtin_cimagf") || equalc(tok, "__builtin_cimagl")) {
        Token *start = tok;
        bool is_imag = equalc(tok, "__builtin_cimag") ||
            equalc(tok, "__builtin_cimagf") ||
            equalc(tok, "__builtin_cimagl");
        tok = skip(tok->next, "(");
        Node *arg = assign(&tok, tok);
        *rest = skip(tok, ")");
        check_type(arg);
        Node *n = new_unary(is_imag ? ND_IMAG : ND_REAL, arg, start);
        check_type(n);
        return n;
    }
    // __builtin_conjf/conj/conjl(z) — complex conjugate: negate the imaginary part
    if (equalc(tok, "__builtin_conjf") || equalc(tok, "__builtin_conj") ||
        equalc(tok, "__builtin_conjl")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *arg = assign(&tok, tok);
        *rest = skip(tok, ")");
        check_type(arg);
        Type *cty = arg->ty;
        if (!is_complex(cty))
            cty = complex_type(ty_double);
        Node *re = new_unary(ND_REAL, arg, start);
        Node *im = new_unary(ND_IMAG, arg, start);
        Node *neg_im = new_unary(ND_NEG, im, start);
        check_type(neg_im);
        return new_complex_val(re, neg_im, cty, start);
    }
    // __builtin_inf/inff/infl and __builtin_huge_val/valf/vall — floating
    // infinity constants usable in constant expressions and static initializers.
    if (equalc(tok, "__builtin_inf") || equalc(tok, "__builtin_huge_val") ||
        equalc(tok, "__builtin_inff") || equalc(tok, "__builtin_huge_valf") ||
        equalc(tok, "__builtin_infl") || equalc(tok, "__builtin_huge_vall")) {
        Token *start = tok;
        Type *fty = ty_double;
        if (equalc(tok, "__builtin_inff") || equalc(tok, "__builtin_huge_valf"))
            fty = ty_float;
        else if (equalc(tok, "__builtin_infl") || equalc(tok, "__builtin_huge_vall"))
            fty = ty_ldouble;
        tok = skip(tok->next, "(");
        *rest = skip(tok, ")");
        Node *n = new_fnum((double)__builtin_inff(), start);
        n->ty = fty;
        return n;
    }
    // __builtin_nan/nanf/nanl (quiet) and __builtin_nans/nansf/nansl (signaling)
    // — NaN constants.  We emit a quiet NaN in all cases; the only distinction
    // that matters for our torture tests is that isnan() is true.
    if (equalc(tok, "__builtin_nan") || equalc(tok, "__builtin_nanf") ||
        equalc(tok, "__builtin_nanl") || equalc(tok, "__builtin_nans") ||
        equalc(tok, "__builtin_nansf") || equalc(tok, "__builtin_nansl")) {
        Token *start = tok;
        Type *fty = ty_double;
        size_t nl = tok->len;
        char last = tok->ptr[nl - 1];
        if (last == 'f')
            fty = ty_float;
        else if (last == 'l')
            fty = ty_ldouble;
        tok = skip(tok->next, "(");
        if (!equalc(tok, ")"))
            assign(&tok, tok); // tag string argument, ignored
        *rest = skip(tok, ")");
        Node *n = new_fnum(__builtin_nan(""), start);
        n->ty = fty;
        return n;
    }
    if (equalc(tok, "__builtin_va_start")) {
        Node *node = new_node(ND_VA_START, tok);
        tok = skip(tok->next, "(");
        node->lhs = assign(&tok, tok);
        // C23: any arguments after the first (the va_list) are not evaluated,
        // and may be arbitrary, ill-formed token sequences.  Skip them without
        // parsing, tracking parenthesis nesting to find the matching ')'.
        if (equalc(tok, ",")) {
            int depth = 0;
            while (tok->kind != TK_EOF) {
                if (equalc(tok, "(")) depth++;
                else if (equalc(tok, ")")) {
                    if (depth == 0) break;
                    depth--;
                }
                tok = tok->next;
            }
        }
        *rest = skip(tok, ")");
        return node;
    }
    if (equalc(tok, "__builtin_va_copy")) {
        Node *node = new_node(ND_VA_COPY, tok);
        tok = skip(tok->next, "(");
        node->lhs = assign(&tok, tok);
        tok = skip(tok, ",");
        node->rhs = assign(&tok, tok);
        *rest = skip(tok, ")");
        return node;
    }
    if (equalc(tok, "__builtin_va_end")) {
        tok = skip(tok->next, "(");
        Node *node = assign(&tok, tok);
        *rest = skip(tok, ")");
        return node;
    }
    // GCC built-ins for forwarding variadic arguments (simplified: parse but treat as 0)
    if (equalc(tok, "__builtin_va_arg_pack")) {
        tok = skip(tok->next, "(");
        *rest = skip(tok, ")");
        // Expanded by the call-site inliner (see inline_pack_fns) into the
        // caller's trailing variadic args. If it survives to codegen
        // (e.g. unused inline-pack function), emits a harmless 0.
        Node *node = new_node(ND_VA_ARG_PACK, tok);
        node->ty = ty_int;
        return node;
    }
    if (equalc(tok, "__builtin_va_arg_pack_len")) {
        tok = skip(tok->next, "(");
        *rest = skip(tok, ")");
        // Expanded by the call-site inliner into the *count* of the caller's
        // trailing variadic args (see clone_inline_node). If it survives to
        // codegen (unused inline-pack function), emits a harmless 0.
        Node *node = new_node(ND_VA_ARG_PACK_LEN, tok);
        node->ty = ty_int;
        return node;
    }
    if (equalc(tok, "__builtin_apply_args")) {
        tok = skip(tok->next, "(");
        *rest = skip(tok, ")");
        return new_num(0, tok); // returns void*, simplified as 0
    }
    if (equalc(tok, "__builtin_apply")) {
        // __builtin_apply(fn, args, size) - simplified: parse args, ignore call
        tok = skip(tok->next, "(");
        Node *fn = assign(&tok, tok);
        (void)fn;
        tok = skip(tok, ",");
        assign(&tok, tok); // args - skip
        tok = skip(tok, ",");
        assign(&tok, tok); // size - skip
        *rest = skip(tok, ")");
        return new_node(ND_NULL, tok);
    }
    if (equalc(tok, "__builtin_return")) {
        // __builtin_return(result) - simplified: parse and ignore
        tok = skip(tok->next, "(");
        assign(&tok, tok);
        *rest = skip(tok, ")");
        return new_node(ND_NULL, tok);
    }
    if (equalc(tok, "__builtin_va_arg")) {
        Node *node = new_node(ND_VA_ARG, tok);
        tok = skip(tok->next, "(");

        Node *ap_arg = assign(&tok, tok);
        check_type(ap_arg);
        node->lhs = ap_arg;
        tok = skip(tok, ",");

        VarAttr attr = {0};
        Type *ty = type_name(&tok, tok);
        (void)attr;
        *rest = skip(tok, ")");

        // C11 6.7.6.2p5 (as extended to __builtin_va_arg's type-name, same
        // class as a VM-typed cast - see vla_freeze_dims): a variably
        // modified type embedded in the requested type (e.g.
        // `int (*)[++i]`) must have its dimension side effects evaluated
        // exactly once, here, not silently dropped.
        Node *vla_pre = NULL;
        if (parser_current_fn && is_vm_type(ty))
            ty = vla_freeze_dims(ty, &vla_pre, tok);

        node->ty = pointer_to(ty);
        node = new_unary(ND_DEREF, node, tok);
        if (vla_pre) {
            check_type(node);
            Node *chain = new_node(ND_COMMA, tok);
            chain->lhs = vla_pre;
            chain->rhs = node;
            check_type(chain);
            // Postfix operators (`.`/`->`/`[]`/`++`/`--`) may chain
            // directly onto va_arg's result -- e.g. `va_arg(ap,
            // BigStruct).c[123]` (slimcc's own variadic test harness).
            // Mirrors __builtin_choose_expr's identical fix just below.
            return apply_postfix_ops(chain, rest, *rest);
        }
        return apply_postfix_ops(node, rest, *rest);
    }
    if (equalc(tok, "__atomic_is_lock_free")) {
        tok = skip(tok->next, "(");
        assign(&tok, tok);
        tok = skip(tok, ",");
        assign(&tok, tok);
        *rest = skip(tok, ")");
        return new_num(1, tok);
    }
    if (equalc(tok, "__atomic_thread_fence")) {
        Node *node = new_node(ND_ATOMIC_FENCE, tok);
        node->atomic_ord = MEMORDER_SEQ_CST;
        tok = skip(tok->next, "(");
        node->atomic_ord = parse_memory_order(&tok);
        *rest = skip(tok, ")");
        return node;
    }
    if (equalc(tok, "__atomic_signal_fence")) {
        Node *node = new_node(ND_ATOMIC_FENCE, tok);
        node->atomic_signal_fence = true;
        node->atomic_ord = MEMORDER_SEQ_CST;
        tok = skip(tok->next, "(");
        node->atomic_ord = parse_memory_order(&tok);
        *rest = skip(tok, ")");
        return node;
    }
    if (equalc(tok, "__atomic_test_and_set")) {
        Token *start = tok;
        Node *node = new_node(ND_ATOMIC_EXCHANGE, start);
        node->atomic_ord = MEMORDER_SEQ_CST;
        tok = skip(tok->next, "(");
        node->lhs = assign(&tok, tok);
        check_type(node->lhs);
        if (equalc(tok, ",")) {
            tok = tok->next;
            node->atomic_ord = parse_memory_order(&tok);
        }
        *rest = skip(tok, ")");
        node->rhs = new_num(1, start);
        node->ty = ty_bool;
        return node;
    }
    if (equalc(tok, "__atomic_clear")) {
        Token *start = tok;
        Node *node = new_node(ND_ATOMIC_STORE, start);
        node->atomic_ord = MEMORDER_SEQ_CST;
        tok = skip(tok->next, "(");
        node->lhs = assign(&tok, tok);
        check_type(node->lhs);
        if (equalc(tok, ",")) {
            tok = tok->next;
            node->atomic_ord = parse_memory_order(&tok);
        }
        *rest = skip(tok, ")");
        node->rhs = new_num(0, start);
        node->ty = ty_void;
        return node;
    }
    if (equalc(tok, "__atomic_load_n") || equalc(tok, "__atomic_load") || atomic_lib_helper(tok, "load")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        if (ptr->ty->kind != TY_PTR && ptr->ty->kind != TY_ARRAY)
            error_tok_simple(start, "pointer expected");
        else {
            Type *base = ptr->ty->base;
            if (!base || base->size == 0 || base->size > 8 || (base->size & (base->size - 1)))
                error_tok_simple(start, "integral or integer-sized pointer target type expected");
        }
        tok = skip(tok, ",");
        Node *ret_ptr = NULL;
        if (!equalc(start, "__atomic_load_n") && !atomic_lib_helper(start, "load")) {
            // __atomic_load(ptr, retptr, order): the 2nd argument is a
            // *pointer to* where the loaded value is stored, unlike
            // __atomic_load_n's 2-argument form where the loaded value
            // is simply the call's own result. Mirrors __atomic_store's
            // identical ptr/valptr asymmetry just below -- without this,
            // retptr was silently misparsed as the memory-order argument.
            ret_ptr = assign(&tok, tok);
            check_type(ret_ptr);
            if (ret_ptr->ty->kind != TY_PTR && ret_ptr->ty->kind != TY_ARRAY)
                error_tok_simple(start, "pointer expected");
            tok = skip(tok, ",");
        }
        Node *node = new_node(ND_ATOMIC_LOAD, start);
        node->lhs = ptr;
        node->atomic_ord = parse_memory_order(&tok);
        *rest = skip(tok, ")");
        if (ptr->ty->base)
            node->ty = ptr->ty->base;
        else
            node->ty = ty_int;
        if (ret_ptr) {
            // *retptr = <loaded value>; the whole expression is void,
            // matching __atomic_store's return type.
            Node *deref = new_unary(ND_DEREF, ret_ptr, start);
            check_type(deref);
            Node *store = new_binary(ND_ASSIGN, deref, node, start);
            check_type(store);
            return store;
        }
        return node;
    }
    if (equalc(tok, "__atomic_store_n") || equalc(tok, "__atomic_store") || atomic_lib_helper(tok, "store")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        if (!equalc(start, "__atomic_store_n") && !atomic_lib_helper(start, "store")) {
            // __atomic_store(ptr, valptr, order): the 2nd argument is a
            // *pointer to* the value to store, unlike __atomic_store_n
            // where it's the value itself. Dereference it so codegen
            // stores *valptr, not valptr's own address -- storing the raw
            // pointer would corrupt the target with the argument's address
            // (e.g. zlib-ng's FUNCTABLE_ASSIGN(VAR, FUNC_NAME) pattern:
            // __atomic_store(&functable.FUNC_NAME, &VAR.FUNC_NAME, ...)
            // must copy *&VAR.FUNC_NAME, not store &VAR.FUNC_NAME itself).
            if (val->ty->kind != TY_PTR && val->ty->kind != TY_ARRAY)
                error_tok_simple(start, "pointer expected");
            val = new_unary(ND_DEREF, val, start);
            check_type(val);
        }
        if (ptr->ty->kind == TY_PTR || ptr->ty->kind == TY_ARRAY) {
            Type *base = ptr->ty->base;
            if (base) {
                if (ty_const(base))
                    warn_tok(start, "assignment of read-only location");
                if (equalc(start, "__atomic_store_n") || atomic_lib_helper(start, "store")) {
                    if (val->ty) {
                        if (is_integer(base) && (val->ty->kind == TY_PTR || val->ty->kind == TY_ARRAY))
                            warn_tok(start, "assignment makes integer from pointer without a cast");
                        else if (base->kind == TY_PTR && (val->ty->kind == TY_PTR || val->ty->kind == TY_ARRAY)) {
                            Type *bbase = base->base, *vbase = val->ty->base;
                            if (bbase && vbase && bbase->kind != TY_VOID && vbase->kind != TY_VOID &&
                                (bbase->kind != vbase->kind || bbase->size != vbase->size))
                                warn_tok(start, "assignment from incompatible pointer type");
                        }
                    }
                }
            }
        }
        Node *node = new_node(ND_ATOMIC_STORE, start);
        node->lhs = ptr;
        node->rhs = val;
        tok = skip(tok, ",");
        node->atomic_ord = parse_memory_order(&tok);
        *rest = skip(tok, ")");
        node->ty = ty_void;
        return node;
    }
    if (equalc(tok, "__atomic_exchange_n") || equalc(tok, "__atomic_exchange") || atomic_lib_helper(tok, "exchange")) {
        Token *start = tok;
        // __atomic_exchange_n(ptr, val, order) and the libatomic sized
        // forms __atomic_exchange_<N> return the old value; the generic
        // __atomic_exchange(ptr, val, ret, order) stores the old value
        // into *ret and is void (GCC semantics). The ret argument was
        // being misparsed as the memory order, blowing up the comma skip
        // with "expected specific operator" (tinycc's 125_atomic_misc).
        bool four_arg = equalc(tok, "__atomic_exchange");
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        if (ptr->ty->kind == TY_PTR || ptr->ty->kind == TY_ARRAY) {
            Type *base = ptr->ty->base;
            if (base && ty_const(base))
                warn_tok(start, "assignment of read-only location");
        }
        Node *ret_ptr = NULL;
        if (four_arg) {
            // __atomic_exchange takes *pointers* to both the value and the
            // old-value slot (unlike _n/_N which take the value directly):
            // dereference them so the exchange uses *val and stores *ret.
            tok = skip(tok, ",");
            ret_ptr = assign(&tok, tok);
            check_type(ret_ptr);
            if (ret_ptr->ty->kind != TY_PTR && ret_ptr->ty->kind != TY_ARRAY)
                error_tok_simple(start, "pointer expected");
            Node *val_deref = new_unary(ND_DEREF, val, start);
            check_type(val_deref);
            val = val_deref;
        }
        Node *node = new_node(ND_ATOMIC_EXCHANGE, start);
        node->lhs = ptr;
        node->rhs = val;
        tok = skip(tok, ",");
        node->atomic_ord = parse_memory_order(&tok);
        *rest = skip(tok, ")");
        if (ptr->ty->base)
            node->ty = ptr->ty->base;
        else
            node->ty = ty_int;
        if (ret_ptr) {
            // *ret = <old value>; the whole expression is void
            Node *deref = new_unary(ND_DEREF, ret_ptr, start);
            check_type(deref);
            Node *store = new_binary(ND_ASSIGN, deref, node, start);
            check_type(store);
            return store;
        }
        return node;
    }
    if (equalc(tok, "__atomic_compare_exchange_n") || equalc(tok, "__atomic_compare_exchange") || atomic_lib_helper(tok, "compare_exchange")) {
        Token *start = tok;
        // __atomic_compare_exchange_n and the libatomic sized
        // __atomic_compare_exchange_<N> forms take the desired value
        // directly (rcc's own test_atomic_libatomic_helpers passes e.g.
        // `__atomic_compare_exchange_4(&x, &expected, 99, ...)`); only the
        // generic __atomic_compare_exchange takes a *pointer to* the desired
        // value (GCC signature: bool f(type *ptr, type **expected,
        // type *desired, bool weak, int success, int failure)). Without the
        // deref the pointer bits were exchanged into *ptr (tinycc's
        // 125_atomic_misc: a became garbage after a strong exchange).
        bool desired_is_ptr = equalc(tok, "__atomic_compare_exchange");
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *expected = assign(&tok, tok);
        check_type(expected);
        tok = skip(tok, ",");
        Node *desired = assign(&tok, tok);
        check_type(desired);
        if (desired_is_ptr) {
            Node *desired_deref = new_unary(ND_DEREF, desired, start);
            check_type(desired_deref);
            desired = desired_deref;
        }
        if ((ptr->ty->kind == TY_PTR || ptr->ty->kind == TY_ARRAY) && ptr->ty->base) {
            Type *base = ptr->ty->base;
            if ((expected->ty->kind == TY_PTR || expected->ty->kind == TY_ARRAY) && expected->ty->base) {
                if (expected->ty->base->size != base->size)
                    error_tok_simple(start, "pointer target type mismatch in argument 2");
            } else {
                error_tok_simple(start, "pointer target type mismatch in argument 2");
            }
        }
        Node *node = new_node(ND_ATOMIC_CAS, start);
        node->lhs = ptr;
        node->body = expected;
        node->rhs = desired;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_ord2 = MEMORDER_SEQ_CST;
        node->atomic_weak = false;
        if (equalc(tok, ",")) {
            tok = tok->next;
            node->atomic_weak = !!parse_memory_order(&tok);
            if (equalc(tok, ",")) {
                tok = tok->next;
                node->atomic_ord = parse_memory_order(&tok);
                if (equalc(tok, ",")) {
                    tok = tok->next;
                    node->atomic_ord2 = parse_memory_order(&tok);
                }
            }
        }
        *rest = skip(tok, ")");
        node->ty = ty_bool;
        return node;
    }
#define ATOMIC_FETCH_OP_HELPER(name_str, op_val) do { \
    Token *start = tok; \
    tok = skip(tok->next, "("); \
    Node *ptr = assign(&tok, tok); \
    check_type(ptr); \
    tok = skip(tok, ","); \
    Node *val = assign(&tok, tok); \
    check_type(val); \
    if (ptr->ty->kind == TY_PTR || ptr->ty->kind == TY_ARRAY) { \
        Type *base = ptr->ty->base; \
        /* GCC allows atomic_fetch_add/sub (ops 0/1) on an atomic pointer
         * object with ordinary pointer-arithmetic scaling by the
         * pointee's pointee size (C11 7.17.7.5); only bitwise ops
         * (or/xor/and/nand) require a genuine integer target. A pointer
         * base only qualifies when the pointer itself is atomic-qualified
         * (`int *_Atomic p`) -- a plain pointer to an atomic pointee
         * (`_Atomic int *p`) is not an atomic pointer object at all. */ \
        bool atomic_ptr_base = base && base->kind == TY_PTR && ty_atomic(base); \
        if (!base || (atomic_ptr_base && (op_val) != 0 && (op_val) != 1) || \
            (base->kind == TY_PTR && !atomic_ptr_base) || \
            (base->kind != TY_PTR && (base->size == 0 || base->size > 8 || (base->size & (base->size - 1))))) \
            error_tok_simple(start, "integral or integer-sized pointer target type expected"); \
        else if (ty_const(base)) \
            warn_tok(start, "assignment of read-only location"); \
    } \
    Node *node = new_node(ND_ATOMIC_FETCH_OP, start); \
    node->lhs = ptr; \
    node->rhs = val; \
    node->atomic_fetch_op = (op_val); \
    tok = skip(tok, ","); \
    node->atomic_ord = parse_memory_order(&tok); \
    *rest = skip(tok, ")"); \
    if (ptr->ty->base) \
        node->ty = ptr->ty->base; \
    else \
        node->ty = ty_int; \
    return node; \
} while(0)
    if (equalc(tok, "__atomic_fetch_add"))
        ATOMIC_FETCH_OP_HELPER("__atomic_fetch_add", 0);
    if (equalc(tok, "__atomic_fetch_sub"))
        ATOMIC_FETCH_OP_HELPER("__atomic_fetch_sub", 1);
    if (equalc(tok, "__atomic_fetch_or"))
        ATOMIC_FETCH_OP_HELPER("__atomic_fetch_or", 2);
    if (equalc(tok, "__atomic_fetch_xor"))
        ATOMIC_FETCH_OP_HELPER("__atomic_fetch_xor", 3);
    if (equalc(tok, "__atomic_fetch_and"))
        ATOMIC_FETCH_OP_HELPER("__atomic_fetch_and", 4);
    if (equalc(tok, "__atomic_fetch_nand"))
        ATOMIC_FETCH_OP_HELPER("__atomic_fetch_nand", 5);
#undef ATOMIC_FETCH_OP_HELPER
    if (equalc(tok, "__atomic_add_fetch")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 0;
        node->atomic_is_store = true;
        if (equalc(tok, ",")) {
            tok = tok->next;
            node->atomic_ord = parse_memory_order(&tok);
        }
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__atomic_sub_fetch")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 1;
        node->atomic_is_store = true;
        if (equalc(tok, ",")) {
            tok = tok->next;
            node->atomic_ord = parse_memory_order(&tok);
        }
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__atomic_or_fetch")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 2;
        node->atomic_is_store = true;
        if (equalc(tok, ",")) {
            tok = tok->next;
            node->atomic_ord = parse_memory_order(&tok);
        }
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__atomic_xor_fetch")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 3;
        node->atomic_is_store = true;
        if (equalc(tok, ",")) {
            tok = tok->next;
            node->atomic_ord = parse_memory_order(&tok);
        }
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__atomic_and_fetch")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 4;
        node->atomic_is_store = true;
        if (equalc(tok, ",")) {
            tok = tok->next;
            node->atomic_ord = parse_memory_order(&tok);
        }
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__atomic_nand_fetch")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 5;
        node->atomic_is_store = true;
        if (equalc(tok, ",")) {
            tok = tok->next;
            node->atomic_ord = parse_memory_order(&tok);
        }
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__sync_synchronize")) {
        Node *node = new_node(ND_ATOMIC_FENCE, tok);
        node->body = NULL;
        node->atomic_ord = MEMORDER_SEQ_CST;
        *rest = skip(tok->next, "(");
        *rest = skip(*rest, ")");
        return node;
    }
    if (equalc(tok, "__sync_lock_test_and_set")) {
        Token *start = tok;
        Node *node = new_node(ND_ATOMIC_EXCHANGE, start);
        node->atomic_ord = MEMORDER_ACQ_REL;
        tok = skip(tok->next, "(");
        node->lhs = assign(&tok, tok);
        tok = skip(tok, ",");
        node->rhs = assign(&tok, tok);
        check_type(node->rhs);
        *rest = skip(tok, ")");
        if (node->lhs->ty && node->lhs->ty->base)
            node->ty = node->lhs->ty->base;
        else
            node->ty = ty_int;
        return node;
    }
    if (equalc(tok, "__sync_lock_release")) {
        Token *start = tok;
        Node *node = new_node(ND_ATOMIC_STORE, start);
        node->atomic_ord = MEMORDER_RELEASE;
        tok = skip(tok->next, "(");
        node->lhs = assign(&tok, tok);
        node->rhs = new_num(0, start);
        *rest = skip(tok, ")");
        node->ty = ty_void;
        return node;
    }
    if (equalc(tok, "__sync_fetch_and_add")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 0;
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__sync_fetch_and_sub")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 1;
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__sync_fetch_and_or")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 2;
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__sync_fetch_and_xor")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 3;
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__sync_fetch_and_and")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 4;
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__sync_fetch_and_nand")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        Node *node = new_node(ND_ATOMIC_FETCH_OP, start);
        node->lhs = ptr;
        node->rhs = val;
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_fetch_op = 5;
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__sync_val_compare_and_swap") || equalc(tok, "__sync_bool_compare_and_swap")) {
        bool is_val = equalc(tok, "__sync_val_compare_and_swap");
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *oldval = assign(&tok, tok);
        check_type(oldval);
        tok = skip(tok, ",");
        Node *newval = assign(&tok, tok);
        check_type(newval);
        Node *node = new_node(ND_ATOMIC_CAS, start);
        node->lhs = ptr;
        node->rhs = newval;
        // Unlike __atomic_compare_exchange, GCC's __sync_*_compare_and_swap
        // take `oldval` BY VALUE -- it may be any expression (a literal,
        // a function call, ...), not necessarily addressable. The codegen
        // needs an address (to load the comparand and, on failure, receive
        // the actual old value), so materialize it into a compiler-
        // synthesized local temp and address THAT instead of `oldval`
        // itself (found via json-c's `__sync_val_compare_and_swap(&seed,
        // -1, seed)`: taking the address of the literal -1 crashed
        // codegen with "Invalid register -1").
        Type *cas_ty = ptr->ty->base ? ptr->ty->base : (oldval->ty ? oldval->ty : ty_int);
        LVar *tmp = new_var("", cas_ty, true);
        Node *tmp_var = new_var_node(tmp, start);
        Node *tmp_assign = new_binary(ND_ASSIGN, tmp_var, oldval, start);
        check_type(tmp_assign);
        node->body = new_unary(ND_ADDR, tmp_var, start);
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_ord2 = MEMORDER_SEQ_CST;
        *rest = skip(tok, ")");
        if (is_val) {
            node->atomic_cas_return_old = true;
            node->ty = cas_ty;
        } else {
            node->ty = ty_bool;
        }
        return new_binary(ND_COMMA, tmp_assign, node, start);
    }
    if (equalc(tok, "__builtin_constant_p")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *arg = assign(&tok, tok);
        *rest = skip(tok, ")");
        check_type(arg);
        long long cv;
        bool is_const = arg->kind == ND_NUM || arg->kind == ND_FNUM || arg->kind == ND_STR || eval_const_expr(arg, &cv);
        return new_num(is_const ? 1 : 0, start);
    }
    // GCC __builtin_shuffle(vec, mask) / __builtin_shuffle(vec0, vec1, mask):
    // per-lane gather r.__v<i> = data[mask.__v<i> & (N-1)]; mask indices are
    // taken modulo N (modulo 2N for the two-vector form, upper half selects
    // from vec1).
    if (equalc(tok, "__builtin_shuffle")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *a1 = assign(&tok, tok);
        tok = skip(tok, ",");
        Node *a2 = assign(&tok, tok);
        Node *a3 = NULL;
        if (equalc(tok, ","))
            a3 = assign(&tok, tok->next);
        *rest = skip(tok, ")");
        check_type(a1);
        check_type(a2);
        if (a3)
            check_type(a3);
        Node *mask = a3 ? a3 : a2;
        if (!a1->ty || !a1->ty->is_vector || !mask->ty || !mask->ty->is_vector)
            error_tok(start, "__builtin_shuffle requires vector arguments");
        Type *vt = a1->ty;
        Type *mt = mask->ty;
        int n = (int)(vt->size / vt->base->size);
        Node *chain = NULL;
        LVar *ta = vec_bind(&chain, a1, vt, start);
        LVar *tb = a3 ? vec_bind(&chain, a2, a2->ty, start) : NULL;
        LVar *tm = vec_bind(&chain, mask, mt, start);
        LVar *tr = new_var("", vt, true);
        for (int i = 0; i < n; i++) {
            Node *val;
            if (!tb) {
                Node *idx = new_binary(ND_BITAND, vec_lane(tm, mt, i, start),
                                       new_num(n - 1, start), start);
                val = vec_gather(ta, vt, idx, start);
            } else {
                // j = mask & (2N-1);  j < N ? vec0[j] : vec1[j - N]
                // (j - N == j & (N-1) for j in [N, 2N) with power-of-two N)
                Node *j = new_binary(ND_BITAND, vec_lane(tm, mt, i, start),
                                     new_num(2 * n - 1, start), start);
                Node *cnode = new_node(ND_COND, start);
                cnode->cond = new_binary(ND_LT, j, new_num(n, start), start);
                cnode->then = vec_gather(
                    ta, vt,
                    new_binary(ND_BITAND, vec_lane(tm, mt, i, start),
                               new_num(2 * n - 1, start), start),
                    start);
                cnode->els = vec_gather(
                    tb, a2->ty,
                    new_binary(ND_BITAND, vec_lane(tm, mt, i, start),
                               new_num(n - 1, start), start),
                    start);
                val = cnode;
            }
            Node *st = new_binary(ND_ASSIGN, vec_lane(tr, vt, i, start), val, start);
            chain = new_binary(ND_COMMA, chain, st, start);
        }
        chain = new_binary(ND_COMMA, chain, new_var_node(tr, start), start);
        check_type(chain);
        return chain;
    }
    // Clang/GCC __builtin_shufflevector(vec1, vec2, i0, i1, ..., iN-1):
    // compile-time-constant-index shuffle. Unlike __builtin_shuffle (a
    // runtime mask vector, output shape == input shape), every index here
    // is a literal constant expression and the output lane count is simply
    // however many indices were given (may differ from either input's lane
    // count). Index i selects vec1[i] for i in [0, N1), or vec2[i - N1] for
    // i in [N1, N1+N2); GCC's own x86 intrinsic headers (avx2intrin.h's
    // _mm_reduce_*/_mm256_reduce_* macros) use this unconditionally on
    // every #include, so it must be parsed even by TUs that never call
    // those functions.
    if (equalc(tok, "__builtin_shufflevector")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *a1 = assign(&tok, tok);
        tok = skip(tok, ",");
        Node *a2 = assign(&tok, tok);
        check_type(a1);
        check_type(a2);
        if (!a1->ty || !a1->ty->is_vector || !a2->ty || !a2->ty->is_vector)
            error_tok(start, "__builtin_shufflevector requires vector arguments");
        Type *vt1 = a1->ty, *vt2 = a2->ty;
        int n1 = (int)(vt1->size / vt1->base->size);
        int n2 = (int)(vt2->size / vt2->base->size);
        long long idxs[64];
        int nidx = 0;
        while (equalc(tok, ",")) {
            if (nidx >= 64)
                error_tok(start, "__builtin_shufflevector: too many indices");
            Node *ie = assign(&tok, tok->next);
            check_type(ie);
            long long v;
            if (!eval_const_expr(ie, &v))
                error_tok(start, "__builtin_shufflevector indices must be constant expressions");
            idxs[nidx++] = v;
        }
        *rest = skip(tok, ")");
        if (nidx == 0)
            error_tok(start, "__builtin_shufflevector requires at least one index");
        Type *rty = make_vector_type(vt1->base, nidx * (int)vt1->base->size);
        Node *chain = NULL;
        LVar *ta = vec_bind(&chain, a1, vt1, start);
        LVar *tb = vec_bind(&chain, a2, vt2, start);
        LVar *tr = new_var("", rty, true);
        for (int i = 0; i < nidx; i++) {
            long long idx = idxs[i];
            Node *val;
            if (idx < 0)
                val = vec_lane(ta, vt1, 0, start); // "don't care" lane; any value is conformant
            else if (idx < n1)
                val = vec_lane(ta, vt1, (int)idx, start);
            else if (idx < n1 + n2)
                val = vec_lane(tb, vt2, (int)(idx - n1), start);
            else
                error_tok(start, "__builtin_shufflevector index out of range");
            Node *st = new_binary(ND_ASSIGN, vec_lane(tr, rty, i, start), val, start);
            chain = new_binary(ND_COMMA, chain, st, start);
        }
        chain = new_binary(ND_COMMA, chain, new_var_node(tr, start), start);
        check_type(chain);
        return chain;
    }
    if (equalc(tok, "__builtin_choose_expr")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *cond = assign(&tok, tok);
        tok = skip(tok, ",");
        Node *expr1 = assign(&tok, tok);
        tok = skip(tok, ",");
        Node *expr2 = assign(&tok, tok);
        tok = skip(tok, ")");
        // If condition is a compile-time constant, return the appropriate branch
        long long cv = 0;
        Node *result;
        if (eval_const_expr(cond, &cv)) {
            result = cv ? expr1 : expr2;
        } else {
            // Non-constant: generate as runtime ternary
            result = new_node(ND_COND, start);
            result->cond = cond;
            result->then = expr1;
            result->els = expr2;
        }
        // A postfix chain may directly follow, e.g.
        // drivers/gpu/drm/i915/gt/intel_gtt.h's px_used(px) macro expands to
        // "&__builtin_choose_expr(...)->used" — the "->used" must still be
        // parsed here, exactly as primary() would for any other expression.
        return apply_postfix_ops(result, rest, tok);
    }
    if (equalc(tok, "__builtin_types_compatible_p")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Type *t1 = type_name(&tok, tok);
        tok = skip(tok, ",");
        Type *t2 = type_name(&tok, tok);
        *rest = skip(tok, ")");
        return new_num(types_compatible_p(t1, t2), start);
    }
    if (equalc(tok, "__builtin_classify_type")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *arg = assign(&tok, tok);
        *rest = skip(tok, ")");
        check_type(arg);
        int cls = 1; // default: integer
        if (arg->ty) {
            switch (arg->ty->kind) {
            case TY_VOID: cls = 9; break;
            case TY_BOOL:
            case TY_CHAR:
            case TY_SHORT:
            case TY_INT:
            case TY_LONG:
            case TY_LLONG: cls = 1; break;
            case TY_FLOAT: cls = 2; break;
            case TY_DOUBLE: cls = 3; break;
            case TY_LDOUBLE: cls = 8; break;
            case TY_PTR: cls = 0; break;
            case TY_ARRAY: cls = 6; break;
            case TY_STRUCT:
            case TY_UNION: cls = 12; break;
            default: cls = 0;
            }
        }
        return new_num(cls, start);
    }
    if (equalc(tok, "++")) {
        Token *start = tok;
        Node *lhs = unary(&tok, tok->next);
        *rest = tok;
        Node *vla = vla_ptr_incdec(lhs, true, false, start);
        if (vla)
            return vla;
        // Must compute the operand's lvalue address only once: `++*p++` would
        // otherwise re-run the side-effecting `p++` if desugared to
        // `lhs = lhs + 1`.
        return new_unary(ND_PRE_INC, lhs, start);
    }
    if (equalc(tok, "--")) {
        Token *start = tok;
        Node *lhs = unary(&tok, tok->next);
        *rest = tok;
        Node *vla = vla_ptr_incdec(lhs, false, false, start);
        if (vla)
            return vla;
        return new_unary(ND_PRE_DEC, lhs, start);
    }
    if (equalc(tok, "+")) {
        // C11 6.5.3.3p2: "The result of the unary + operator is the value
        // of its (promoted) operand" -- a narrower-than-int integer
        // operand (char/short/bool/bit-field) undergoes the same integer
        // promotion it would as an arithmetic operand, becoming a real
        // `int` rvalue. This matters for anything that inspects the
        // *type* of `+x` rather than just its value: gnulib's
        // INT_PROMOTE(e) macro (intprops.h) is exactly `(+(e))`, used via
        // `_Generic(INT_PROMOTE((short)0), int: ...)` and
        // `sizeof(INT_PROMOTE((short)0)) == sizeof(int)` to verify this
        // very promotion. Previously unary `+` was a complete no-op
        // (returned the operand unchanged, keeping its narrow type),
        // silently failing both checks. _BitInt is explicitly excluded
        // (C23 6.3.1.1p2: never subject to the usual integer promotions).
        Token *start = tok;
        Node *operand = unary(rest, tok->next);
        check_type(operand);
        // C11 6.5.3.3p1: unary + requires an operand of arithmetic type.
        // nullptr_t is neither arithmetic nor a pointer, so `+nullptr` is
        // a constraint violation (unlike `+ptr`, which GNU/most compilers
        // tolerate as an extension -- narrowly scoped to nullptr_t only).
        if (operand->ty && operand->ty->kind == TY_NULLPTR_T)
            error_tok(start, "wrong type argument to unary plus");
        if (operand->ty && is_integer(operand->ty) && operand->ty->kind != TY_BITINT &&
            operand->ty->size > 0 && operand->ty->size < ty_int->size) {
            Node *promoted = new_unary(ND_CAST, operand, start);
            promoted->ty = ty_int;
            return promoted;
        }
        return operand;
    }
    if (equalc(tok, "-"))
        return vector_lower(new_unary(ND_NEG, unary(rest, tok->next), tok));
    if (equalc(tok, "!"))
        return new_unary(ND_NOT, unary(rest, tok->next), tok);
    if (equalc(tok, "~"))
        return vector_lower(new_unary(ND_BITNOT, unary(rest, tok->next), tok));
    if (equalc(tok, "&")) {
        Node *node = unary(rest, tok->next);
        check_type(node);
        if (node->kind == ND_LVAR && node->var && node->var->is_register)
            error_tok(tok, "address of register compound literal requested");
        // `&fn`: same address-taken tracking as append_reloc() (static
        // initializers) below, but for a runtime expression -- e.g.
        // mpack's own test suite compares `fn_mpack_tag_nil ==
        // &mpack_tag_nil` inside a function body, which never goes
        // through a global initializer's relocation path at all. See
        // rcc.h's LVar.addr_taken doc comment.
        if (node->kind == ND_LVAR && node->var && node->var->is_function)
            node->var->addr_taken = true;
        return new_unary(ND_ADDR, node, tok);
    }
    if (equalc(tok, "*"))
        return new_unary(ND_DEREF, unary(rest, tok->next), tok);
    if (equalc(tok, "sizeof")) {
        if (equalc(tok->next, "(") && is_typename(tok->next->next)) {
            // Check whether this is `sizeof (type) {`, i.e. a compound literal.
            // Walk past the cast type to see if `{` follows the closing `)`.
            Token *t = tok->next; // '('
            int depth = 0;
            for (;;) {
                if (equalc(t, "(")) depth++;
                else if (equalc(t, ")")) {
                    if (--depth == 0) {
                        t = t->next;
                        break;
                    }
                }
                t = t->next;
            }
            if (equalc(t, "{"))
                goto sizeof_expr;
            Token *sty_tok = tok->next->next;
            Type *ty = parse_cast_type(&tok, tok->next);
            // C11 6.5.3.4p1: sizeof shall not apply to an incomplete
            // type. An opaque forward-declared struct/union (no `{...}`
            // body ever seen -- size==0 and no members, distinct from a
            // genuinely zero-sized completed type, which C doesn't have)
            // must be a hard error, not silently sized as 0: iffe (ast/
            // ksh93's own configure-time probe generator) relies on
            // exactly this rejection to tell an opaque struct apart from
            // a real one.
            if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && !ty->has_body)
                error_tok(sty_tok, "invalid application of 'sizeof' to incomplete type");
            *rest = tok;
            if (ty->kind == TY_VLA) {
                // Runtime sizeof for VLA: len * base_size. new_num()'s
                // suffix-sniffing can't be trusted here either (see
                // type_size_node's comment above) -- both operands, and
                // thus the product, must be size_t (unsigned).
                Node *len = ty->vla_len_expr ? ty->vla_len_expr : new_num(ty->array_len, tok);
                if (!ty->vla_len_expr) len->ty = size_t_type();
                Node *base_sz = new_num(ty->base->size, tok);
                base_sz->ty = size_t_type();
                Node *result = new_binary(ND_MUL, len, base_sz, tok);
                check_type(result);
                return result;
            }
            // VLA-containing struct: vla_len_expr holds the full runtime size expr
            if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->vla_len_expr) {
                check_type(ty->vla_len_expr);
                return ty->vla_len_expr;
            }
            {
                Node *n = new_num(ty->size, tok);
                n->ty = size_t_type(); // sizeof is always size_t (unsigned)
                return n;
            }
        }
    sizeof_expr:;
        Node *node = unary(&tok, tok->next);
        check_type(node);
        *rest = tok;
        // Same incomplete-type rejection as the sizeof(type-name) form
        // above, for `sizeof expr`/`sizeof(expr)` where expr's own type
        // turns out to be an opaque struct/union (e.g. `sizeof(i)` where
        // `i` was declared `static OPAQUE i;` — a declaration that is
        // itself invalid for the same reason, but rcc doesn't yet reject
        // that at declaration time, so this is the last line of defense).
        if (node->ty && (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) &&
            !node->ty->has_body)
            error_tok(node->tok, "invalid application of 'sizeof' to incomplete type");
        if (node->ty->kind == TY_VLA) {
            // C11 6.5.3.4p2: sizeof evaluates its operand when the
            // operand's type is itself a VLA (not merely a pointer/array
            // chain reaching one - that's the coarser "variably modified"
            // check typeof/casts use). The runtime size still comes from
            // the *type's* captured length expr, but the operand node
            // itself must also run for its side effects (e.g. sizeof(*p)
            // where p was advanced via a comma operator).
            Node *len = node->ty->vla_len_expr ? node->ty->vla_len_expr : new_num(node->ty->array_len, tok);
            if (!node->ty->vla_len_expr) len->ty = size_t_type();
            Node *base_sz = new_num(node->ty->base->size, tok);
            base_sz->ty = size_t_type();
            Node *result = new_binary(ND_MUL, len, base_sz, tok);
            check_type(result);
            Node *seq = new_binary(ND_COMMA, node, result, tok);
            check_type(seq);
            return seq;
        }
        if ((node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) && node->ty->vla_len_expr) {
            check_type(node->ty->vla_len_expr);
            return node->ty->vla_len_expr;
        }
        {
            Node *n = new_num(node->ty->size, tok);
            n->ty = size_t_type(); // sizeof is always size_t (unsigned)
            return n;
        }
    }
    if (equalc(tok, "__real__") || equalc(tok, "__real")) {
        Node *node = new_unary(ND_REAL, unary(rest, tok->next), tok);
        check_type(node);
        return node;
    }
    if (equalc(tok, "__imag__") || equalc(tok, "__imag")) {
        Node *node = new_unary(ND_IMAG, unary(rest, tok->next), tok);
        check_type(node);
        return node;
    }
    if (equalc(tok, "__alignof__") || equalc(tok, "__alignof") || equalc(tok, "_Alignof") || equalc(tok, "alignof")) {
        Token *start = tok;
        // The standard spellings require a parenthesized type name;
        // __alignof__/__alignof on expressions is the GNU extension.
        bool std_spelling = equalc(tok, "_Alignof");
        if (equalc(tok->next, "(") && is_typename(tok->next->next)) {
            Token *aty_tok = tok->next->next;
            Type *ty = parse_cast_type(&tok, tok->next);
            // C11 6.5.3.4p1: no function or incomplete types.
            // Enforced with -pedantics/-Werror; gcc accepts as extension.
            if (ty->kind == TY_FUNC)
                warn_tok(aty_tok, "'_Alignof' applied to a function type");
            if (ty->kind == TY_VOID || ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && !ty->has_body))
                warn_tok(aty_tok, "'_Alignof' applied to an incomplete type");
            *rest = tok;
            Node *n = new_num(ty->align, start);
            n->ty = size_t_type(); // _Alignof returns size_t (unsigned)
            return n;
        }
        if (opt_pedantic && std_spelling)
            warn_tok(tok->next, "'_Alignof' applied to an expression");
        Node *node = unary(&tok, tok->next);
        check_type(node);
        *rest = tok;
        int al = node->ty->align;
        // For function pointers, use the function type's alignment
        if (node->ty->kind == TY_PTR && node->ty->base && node->ty->base->kind == TY_FUNC)
            al = node->ty->base->align;
        Node *n2 = new_num(al, start);
        n2->ty = size_t_type(); // _Alignof returns size_t (unsigned)
        return n2;
    }
    // C2Y (N3369/N3469) `_Countof`: yields the element count of its
    // operand's outer array dimension as size_t. Non-array operand is a
    // hard error (unlike sizeof/_Alignof, there's no "apply to anything"
    // fallback -- `_Countof` only ever means "how many elements"). VLA
    // dimensions reuse the same runtime vla_len_expr machinery ND_SIZEOF
    // already relies on above, just without the *base->size multiply
    // (a count, not a byte size).
    if (equalc(tok, "_Countof")) {
        Token *start = tok;
        if (equalc(tok->next, "(") && is_typename(tok->next->next)) {
            Token *cty_tok = tok->next->next;
            Type *ty = parse_cast_type(&tok, tok->next);
            *rest = tok;
            if (ty->kind != TY_ARRAY && ty->kind != TY_VLA)
                error_tok(cty_tok, "invalid application of '_Countof' to a non-array type");
            if (ty->kind == TY_VLA) {
                Node *len = ty->vla_len_expr ? ty->vla_len_expr : new_num(ty->array_len, start);
                if (!ty->vla_len_expr) len->ty = size_t_type();
                check_type(len);
                return len;
            }
            Node *n = new_num(ty->size / ty->base->size, start);
            n->ty = size_t_type();
            return n;
        }
        Node *node = unary(&tok, tok->next);
        check_type(node);
        *rest = tok;
        if (node->ty->kind != TY_ARRAY && node->ty->kind != TY_VLA)
            error_tok(node->tok, "invalid application of '_Countof' to a non-array type");
        if (node->ty->kind == TY_VLA) {
            Node *len = node->ty->vla_len_expr ? node->ty->vla_len_expr : new_num(node->ty->array_len, start);
            if (!node->ty->vla_len_expr) len->ty = size_t_type();
            check_type(len);
            // Operand must still run for its side effects (matches
            // ND_SIZEOF's identical VLA rule, C11 6.5.3.4p2).
            Node *seq = new_binary(ND_COMMA, node, len, start);
            check_type(seq);
            return seq;
        }
        Node *n = new_num(node->ty->size / node->ty->base->size, start);
        n->ty = size_t_type();
        return n;
    }
    if (is_cast(tok)) {
        Token *start = tok;
        Type *ty = parse_cast_type(&tok, tok);

        // codeql[cpp/commented-out-code]: doc comment showing the compound-literal syntax parsed below, not dead code
        // Compound literal: (type){init_list}
        if (equalc(tok, "{")) {
            Token *init_brace_tok = tok;
            tok = tok->next;

            // For incomplete arrays, count elements first. Delegate to
            // count_array_initializer() (used elsewhere for the same
            // purpose) rather than a naive top-level-comma count: a naive
            // count silently mis-sizes the array whenever a designator is
            // present, since "[0 ... 2] = 7, [3] = 9" is one comma-
            // separated item covering three indices, not one slot — e.g.
            // drm/intel/pick.h's _PICK() macro relies on exactly this
            // range-designator shape to build a lookup table.
            if (ty->kind == TY_ARRAY && ty->size == 0 && ty->base) {
                Token *tmp = init_brace_tok;
                int count = count_array_initializer(&tmp, tmp, ty->base);
                ty = array_of(ty->base, count);
            }

            // C23: detect storage class specifiers in compound literal types
            // (static, register, thread_local, _Thread_local), plus
            // `constexpr` (gates the "braces around scalar initializer"
            // warning below -- see in_constexpr_init's own comment).
            bool is_storage = false;
            bool is_tls = false;
            bool is_register_cl = false;
            bool has_static_cl = false;
            bool has_constexpr_cl = false;
            for (Token *t = start->next; t && !equalc(t, ")"); t = t->next) {
                if (equalc(t, "static")) {
                    is_storage = true;
                    has_static_cl = true;
                }
                if (equalc(t, "register")) {
                    is_storage = true;
                    is_register_cl = true;
                }
                if (equalc(t, "_Thread_local") || equalc(t, "thread_local")) {
                    is_storage = true;
                    is_tls = true;
                }
                if (equalc(t, "constexpr"))
                    has_constexpr_cl = true;
            }
            bool saved_in_constexpr_init = in_constexpr_init;
            if (has_constexpr_cl)
                in_constexpr_init = true;
            if (current_block_depth > 0 && is_tls && !has_static_cl)
                error_tok(start, "compound literal implicitly auto and declared 'thread_local'");
            static int anon_count;
            char *name = format(".Lanon.%d", anon_count++);
            LVar *var;
            // C11 6.5.2.5p10: a compound literal occurring OUTSIDE the
            // body of a function has STATIC storage duration even
            // without an explicit `static` keyword (only inside a
            // function/block does it default to automatic). Without
            // this, a non-static compound literal reached through this
            // general expression path while still parsing a global
            // initializer got materialized as a fake LOCAL object,
            // whose address extract_reloc() correctly refuses to fold
            // into a link-time relocation (is_local vars have no
            // compile-time address) -- e.g. njs's
            // "(uintptr_t) &(njs_webcrypto_algorithm_t){...}" nested
            // inside a static njs_webcrypto_entry_t[] array element.
            //
            // Gated on in_global_var_init (are we inside
            // global_initializer()'s call tree for a genuine
            // static/global object?), NOT bare current_block_depth == 0:
            // a function prototype's parameter-list array-size
            // expression is ALSO parsed at block depth 0 but is never
            // reached through global_initializer() -- a compound
            // literal there gets automatic storage per C23 6.5.2.5p10
            // regardless (torture/c23-complit-1.c's
            // "void f(int a[(int){x}]);" -- this must stay a genuine
            // local, not a bogus "constant" fold of a mutable global).
            bool is_file_scope = in_global_var_init && !is_register_cl;
            if (is_storage || is_file_scope) {
                var = new_var(name, ty, false);
                if (is_register_cl)
                    var->is_register = true;
                else
                    var->is_static = true;
                var->is_tls = is_tls;
            } else {
                var = new_var(name, ty, true);
            }
            // Storage-class (or implicitly-static file-scope) compound
            // literal: initialize at compile time.
            if (is_storage || is_file_scope) {
                tok = init_brace_tok;
                global_initializer(&tok, tok, var);
            }
            Node *result = new_var_node(var, start);
            if (is_storage || is_file_scope)
                goto apply_postfix;
            // Zero-initialize aggregate compound literal like regular locals (C99 6.7.8p10)
            if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION ||
                 var->ty->kind == TY_ARRAY) &&
                var->ty->size > 0) {
                Node *zinit = new_node(ND_ZERO_INIT, start);
                zinit->lhs = new_var_node(var, start);
                result = new_binary(ND_COMMA, zinit, result, start);
            }

            if (ty->kind == TY_ARRAY && ty->base) {
                // Array compound literal: assign each element
                int i = 0;
                while (!equalc(tok, "}")) {
                    int sidx = i, eidx = i;
                    // Designated initializer: [N] = val or [N ... M] = val
                    // (C99 6.7.8p17) — e.g. drm/intel/pick.h's _PICK()
                    // macro: "(const u32[]){ [TRANSCODER_EDP] = ...,
                    // [TRANSCODER_A] = ..., ... }".
                    if (equalc(tok, "[")) {
                        tok = tok->next;
                        Node *n = assign(&tok, tok);
                        long long sv = 0;
                        eval_const_expr(n, &sv);
                        sidx = (int)sv;
                        eidx = sidx;
                        if (equalc(tok, "...")) {
                            tok = tok->next;
                            Node *n2 = assign(&tok, tok);
                            long long ev = sidx;
                            eval_const_expr(n2, &ev);
                            eidx = (int)ev;
                        }
                        tok = skip(tok, "]");
                        tok = skip(tok, "=");
                    }
                    Token *val_start = tok;
                    for (int j = sidx; j <= eidx; j++) {
                        Node *idx = new_num(j, start);
                        Node *elem_ptr = new_binary(ND_ADD, new_var_node(var, start), idx, start);
                        Node *deref = new_unary(ND_DEREF, elem_ptr, start);
                        Node *val;
                        // {.member=val,...} as nested struct/union initializer for element
                        if (equalc(tok, "{") && (ty->base->kind == TY_STRUCT || ty->base->kind == TY_UNION)) {
                            val = synth_struct_elem_literal(ty->base, &tok, tok, start, &anon_count);
                        } else {
                            val = assign(&tok, tok);
                        }
                        check_type(val);
                        Node *asgn = new_binary(ND_ASSIGN, deref, val, start);
                        check_type(asgn);
                        result = new_binary(ND_COMMA, result, asgn, start);
                        check_type(result);
                        // Reset tok for ranged initializer re-evaluation
                        if (j < eidx)
                            tok = val_start;
                    }
                    i = eidx + 1;
                    if (!equalc(tok, "}"))
                        tok = skip(tok, ",");
                }
                tok = tok->next; // skip }
                // Final value is the array (decays to pointer)
                // Re-wrap so the last value is the variable itself
                Node *final_var = new_var_node(var, start);
                result = new_binary(ND_COMMA, result, final_var, start);
                check_type(result);
                // Compound literals are lvalues; preserve the array type
                // so sizeof and other operations see the correct size.
                result->ty = var->ty;
            } else if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
                // Struct compound literal: assign each member
                // Support designated initializers: .member = value
                Member *mem = ty->members;
                while (!equalc(tok, "}") && (mem || (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT))) {
                    // Designated initializer: .member[.sub]... [= value]
                    if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                        Member *found = NULL;
                        Type *cur_ty = ty;
                        int chain_offset = 0;
                        Token *save = tok;
                        while (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                            char *mname = tok->next->name;
                            // find_member_by_name recurses through anonymous
                            // struct/union members (e.g. `.ubuf` reachable
                            // only via an unnamed union), returning a
                            // synthetic member with the combined offset.
                            Member *m = find_member_by_name(cur_ty, mname);
                            if (!m) break;
                            found = m;
                            chain_offset += m->offset;
                            cur_ty = m->ty;
                            tok = tok->next->next;
                        }
                        if (found) {
                            // A multi-level EXPLICIT designator chain
                            // (".bits.i", as opposed to
                            // find_member_by_name()'s own single-step
                            // anonymous-member recursion above) walks
                            // through a NAMED intermediate member (e.g. a
                            // struct's own named `union { ... } bits;`)
                            // whose offset within the outer struct must be
                            // added on top of the leaf member's own offset
                            // within ITS immediate parent -- `found` alone
                            // only ever carries that immediate-parent-
                            // relative offset (here 0 for "bits.i", since
                            // every union member sits at offset 0 within
                            // the union), never `var`-relative. Without
                            // this accumulation, ".bits.i = val" silently
                            // wrote `val` at offset 0 of the WHOLE struct,
                            // aliasing (and corrupting) any unrelated
                            // member that happens to start there too --
                            // e.g. qbe's `Con.type`, via
                            // "(Con){.type = CBits, .bits.i = val}".
                            if (chain_offset != found->offset) {
                                Member *syn = arena_alloc(sizeof(Member));
                                *syn = *found;
                                syn->offset = chain_offset;
                                found = syn;
                            }
                            mem = found;
                            // A combined `.member[idx] = val` designator (C99
                            // 6.7.8p17) leaves `[idx]` unconsumed here for an
                            // array-typed member — the array-element loop
                            // below (mem->ty->kind == TY_ARRAY) already knows
                            // how to parse `[idx] = val` including the `=`.
                            // Blindly skipping "=" here (as for a scalar/
                            // struct member) would choke on the `[` instead.
                            if (!(mem->ty->kind == TY_ARRAY && equalc(tok, "[")))
                                tok = skip(tok, "=");
                        } else {
                            tok = save; // restore for error recovery
                        }
                    }
                    if (mem->ty->kind == TY_ARRAY) {
                        // String literal → char array: assign whole array at once
                        if (tok->kind == TK_STR && mem->ty->base && mem->ty->base->kind == TY_CHAR) {
                            Node *var_node = new_var_node(var, start);
                            Node *member_access = new_node(ND_MEMBER, start);
                            member_access->lhs = var_node;
                            member_access->member = mem;
                            member_access->ty = mem->ty;
                            Node *rhs = assign(&tok, tok);
                            check_type(rhs);
                            Node *asgn = new_binary(ND_ASSIGN, member_access, rhs, start);
                            check_type(asgn);
                            result = new_binary(ND_COMMA, result, asgn, start);
                            result->ty = ty;
                            if (!equalc(tok, "}"))
                                tok = skip(tok, ",");
                            mem = mem->next;
                            continue;
                        }
                        // Array member: assign elements, handle optional braces
                        int len = array_len(mem->ty);
                        int idx = 0;
                        bool arr_brace = equalc(tok, "{");
                        if (arr_brace) tok = tok->next;
                        while (idx < len && !equalc(tok, "}")) {
                            int sidx = idx, eidx = idx;
                            // Designated initializer: [N] = val or [N ... M] = val
                            Member *elem_mem = NULL;
                            Type *elem_chain_ty = mem->ty->base;
                            int elem_chain_offset = 0;
                            if (equalc(tok, "[")) {
                                tok = tok->next;
                                Node *n = assign(&tok, tok);
                                long long sv = 0;
                                eval_const_expr(n, &sv);
                                sidx = (int)sv;
                                eidx = sidx;
                                if (equalc(tok, "...")) {
                                    tok = tok->next;
                                    Node *n2 = assign(&tok, tok);
                                    long long ev = sidx;
                                    eval_const_expr(n2, &ev);
                                    eidx = (int)ev;
                                }
                                tok = skip(tok, "]");
                                // A struct/union array element may itself
                                // carry a chained member designator:
                                // ".args[0].type = val" (C99 6.7.8p17's
                                // designator chain continuing through an
                                // array-index step) -- e.g. kefir's
                                // DEF_OPCODE macros' compound literals.
                                // Walk it to a leaf member instead of
                                // demanding "=" right after "]".
                                while (elem_chain_ty &&
                                       (elem_chain_ty->kind == TY_STRUCT || elem_chain_ty->kind == TY_UNION) &&
                                       equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                                    Member *sm = find_member_by_name(elem_chain_ty, tok->next->name);
                                    if (!sm) break;
                                    elem_mem = sm;
                                    elem_chain_offset += sm->offset;
                                    elem_chain_ty = sm->ty;
                                    tok = tok->next->next;
                                }
                                tok = skip(tok, "=");
                                idx = sidx;
                            }
                            for (int i = sidx; i <= eidx; i++) {
                                if (len == 0 || i < len) {
                                    Node *var_node = new_var_node(var, start);
                                    Node *member_access = new_node(ND_MEMBER, start);
                                    member_access->lhs = var_node;
                                    member_access->member = mem;
                                    member_access->ty = mem->ty;
                                    Node *offset = new_num(i, start);
                                    Node *elem_ptr = new_binary(ND_ADD, member_access, offset, start);
                                    Node *elem_lhs = new_unary(ND_DEREF, elem_ptr, start);
                                    if (elem_mem) {
                                        check_type(elem_lhs);
                                        Member *leaf = elem_mem;
                                        if (elem_chain_offset != leaf->offset) {
                                            Member *syn = arena_alloc(sizeof(Member));
                                            *syn = *leaf;
                                            syn->offset = elem_chain_offset;
                                            leaf = syn;
                                        }
                                        Node *sub_access = new_node(ND_MEMBER, start);
                                        sub_access->lhs = elem_lhs;
                                        sub_access->member = leaf;
                                        sub_access->ty = leaf->ty;
                                        elem_lhs = sub_access;
                                    }
                                    Type *elem_val_ty = elem_mem ? elem_chain_ty : mem->ty->base;
                                    Token *val_start = tok;
                                    Node *val = (equalc(tok, "{") && (elem_val_ty->kind == TY_STRUCT || elem_val_ty->kind == TY_UNION))
                                        ? synth_struct_elem_literal(elem_val_ty, &tok, tok, start, &anon_count)
                                        : assign(&tok, tok);
                                    check_type(val);
                                    Node *asgn = new_binary(ND_ASSIGN, elem_lhs, val, start);
                                    check_type(asgn);
                                    result = new_binary(ND_COMMA, result, asgn, start);
                                    result->ty = ty;
                                    // Reset tok for ranged initializer re-evaluation
                                    if (i < eidx)
                                        tok = val_start;
                                }
                            }
                            idx = eidx + 1;
                            if (equalc(tok, ",")) {
                                // A fresh top-level ".member" designator
                                // after the comma (rather than a bare
                                // "[idx]=val" continuation of this same
                                // array) restarts member dispatch — e.g.
                                // drivers/scsi/virtio_scsi.c's ".lun[0] =
                                // 1, .lun[1] = ..., .lun[2] = ..., .lun[3]
                                // = ...", which repeats ".lun" per index
                                // instead of chaining bare "[N]=val"
                                // entries after one ".lun". Leave the
                                // comma itself unconsumed so the outer
                                // member loop's own trailing
                                // skip(tok, ",") still applies exactly once.
                                if (equalc(tok->next, ".") && tok->next->next &&
                                    tok->next->next->kind == TK_IDENT)
                                    break;
                                tok = tok->next;
                                if (equalc(tok, "}"))
                                    break;
                                continue;
                            }
                            break;
                        }
                        if (arr_brace) tok = skip(tok, "}");
                    } else if ((mem->ty->kind == TY_STRUCT || mem->ty->kind == TY_UNION) && equalc(tok, "{")) {
                        // Struct/union member with brace-enclosed initializer
                        // (recurses for further nested struct/union members).
                        Node *var_node = new_var_node(var, start);
                        result = assign_nested_struct_init(result, var_node, mem, &tok, tok, start, &anon_count);
                        result->ty = ty;
                    } else if (equalc(tok, "{")) {
                        if (has_constexpr_cl)
                            warn_tok(tok, "braces around scalar initializer");
                        tok = skip(tok, "{");
                        Node *var_node = new_var_node(var, start);
                        Node *member_access = new_node(ND_MEMBER, start);
                        member_access->lhs = var_node;
                        member_access->member = mem;
                        member_access->ty = mem->ty;
                        Node *val;
                        if (equalc(tok, "}")) {
                            val = new_num(0, start);
                        } else {
                            val = assign(&tok, tok);
                            check_type(val);
                        }
                        Node *asgn = new_binary(ND_ASSIGN, member_access, val, start);
                        asgn->ty = mem->ty;
                        result = new_binary(ND_COMMA, result, asgn, start);
                        result->ty = ty;
                        tok = skip(tok, "}");
                    } else {
                        Node *var_node = new_var_node(var, start);
                        Node *member_access = new_node(ND_MEMBER, start);
                        member_access->lhs = var_node;
                        member_access->member = mem;
                        member_access->ty = mem->ty;
                        Node *val = assign(&tok, tok);
                        check_type(val);
                        Node *asgn = new_binary(ND_ASSIGN, member_access, val, start);
                        // Type via check_type so the ND_ASSIGN typing rule
                        // inserts the implicit conversion cast (e.g. int->float
                        // for `(struct S){1,2}` with float members, C11 6.7.9p11).
                        check_type(asgn);
                        result = new_binary(ND_COMMA, result, asgn, start);
                        result->ty = ty;
                    }
                    mem = mem->next;
                    if (!equalc(tok, "}"))
                        tok = skip(tok, ",");
                }
                while (!equalc(tok, "}")) {
                    // Skip extra initializers
                    if (equalc(tok, ",")) {
                        tok = tok->next;
                        continue;
                    }
                    assign(&tok, tok);
                    if (!equalc(tok, "}"))
                        tok = skip(tok, ",");
                }
                tok = tok->next; // skip }
                Node *final_var = new_var_node(var, start);
                result = new_binary(ND_COMMA, result, final_var, start);
                check_type(result);
            } else {
                // Scalar compound literal
                // C23 empty initializer `(T){}`: the value is zero.
                Node *val = equalc(tok, "}") ? new_num(0, start) : assign(&tok, tok);
                check_type(val);
                if (equalc(tok, ",")) tok = tok->next;
                tok = skip(tok, "}");
                Node *asgn = new_binary(ND_ASSIGN, new_var_node(var, start), val, start);
                asgn->ty = ty;
                result = new_binary(ND_COMMA, asgn, new_var_node(var, start), start);
                // Mark anonymous var as constexpr for eval_const_expr
                if (val->kind == ND_NUM) {
                    var->is_constexpr = true;
                    var->has_init = true;
                    var->init_val = val->val;
                }
                check_type(result);
            }

            // Apply postfix operators: compound literals are lvalues that can be subscripted
        apply_postfix:
            while (true) {
                if (equalc(tok, "[")) {
                    Token *ps = tok;
                    Node *idx = expr(&tok, tok->next);
                    tok = skip(tok, "]");
                    result = new_unary(ND_DEREF, new_binary(ND_ADD, result, idx, ps), ps);
                    check_type(result);
                    continue;
                }
                if (equalc(tok, ".")) {
                    tok = tok->next;
                    check_type(result);
                    Member *mem = find_member(result->ty, tok);
                    if (!mem) error_tok(tok, "no such member");
                    Node *mn = new_unary(ND_MEMBER, result, tok);
                    mn->member = mem;
                    mn->ty = mem->ty;
                    result = mn;
                    tok = tok->next;
                    continue;
                }
                if (equalc(tok, "->")) {
                    tok = tok->next;
                    check_type(result);
                    result = new_unary(ND_DEREF, result, tok);
                    check_type(result);
                    Member *mem = find_member(result->ty, tok);
                    if (!mem) error_tok(tok, "no such member");
                    Node *mn = new_unary(ND_MEMBER, result, tok);
                    mn->member = mem;
                    mn->ty = mem->ty;
                    result = mn;
                    tok = tok->next;
                    continue;
                }
                break;
            }
            // Populate init_data for compile-time member access (e.g. in static_assert).
            // Only re-parse with global_initializer if the initializer is all-constant
            // Populate init_data for compile-time member access (e.g. in static_assert).
            // Only call global_initializer when we can guarantee no errors from runtime values.
            // For now, only handle the case where the init tokens are all numeric constants,
            // nullptr, or constexpr variable references — detected by simple token scan.
            if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
                bool all_const = true;
                int brace_depth = 0;
                // codeql[cpp/loop-variable-changed]: deliberate t = t->next skip-ahead past a designated-init member name / parenthesized compound-literal cast (2 sites below)
                for (Token *t = init_brace_tok; t; t = t->next) {
                    if (equalc(t, "{")) {
                        brace_depth++;
                        continue;
                    }
                    if (equalc(t, "}")) {
                        brace_depth--;
                        if (brace_depth <= 0) break;
                        continue;
                    }
                    if (equalc(t, ".")) {
                        if (t->next) t = t->next;
                        continue;
                    } // designated init .member
                    if (equalc(t, ",") || equalc(t, "=") || equalc(t, ";")) continue;
                    if (equalc(t, "(")) { // skip to matching ) — handles compound literal types
                        int pd = 1;
                        while (pd > 0 && t->next) {
                            t = t->next;
                            if (equalc(t, "(")) pd++;
                            else if (equalc(t, ")"))
                                pd--;
                        }
                        continue;
                    }
                    if (t->kind == TK_NUM || t->kind == TK_FNUM || t->kind == TK_STR) continue;
                    if (equalc(t, "nullptr") || equalc(t, "true") || equalc(t, "false") || equalc(t, "NULL")) continue;
                    if (t->kind == TK_IDENT) {
                        // Check global and local constexpr variables
                        LVar *gv = find_global_name(t->name);
                        if (gv && gv->is_constexpr && gv->init_data) continue;
                        bool found = false;
                        for (LVar *lv = locals; lv; lv = lv->next) {
                            if (lv->name == t->name && lv->is_constexpr && lv->init_data) {
                                found = true;
                                break;
                            }
                        }
                        if (found) continue;
                    }
                    all_const = false;
                    break;
                }
                if (all_const) {
                    Token *saved_here = tok;
                    tok = init_brace_tok;
                    bool saved_speculative = in_speculative_const_fold;
                    bool saved_fold_failed = speculative_fold_failed;
                    in_speculative_const_fold = true;
                    speculative_fold_failed = false;
                    global_initializer(&tok, tok, var);
                    bool fold_ok = !speculative_fold_failed;
                    in_speculative_const_fold = saved_speculative;
                    speculative_fold_failed = saved_fold_failed;
                    if (var->has_init && fold_ok)
                        var->is_constexpr = true;
                    else
                        var->has_init = false;
                    tok = saved_here;
                }
            }
            in_constexpr_init = saved_in_constexpr_init;
            *rest = tok;
            return result;
        }

        Node *lhs = unary(rest, tok);
        check_type(lhs);
        // C23: an explicit cast to nullptr_t is only meaningful for a
        // value that could already legally denote a null pointer -- a
        // null pointer constant, or another nullptr_t value. Casting an
        // arbitrary scalar (a float, or an integer constant expression
        // broken by an intervening non-integer cast like
        // `(int)(float)0.0`) to nullptr_t is a constraint violation.
        if (ty->kind == TY_NULLPTR_T && !is_null_value_or_nullptr(lhs))
            error_tok(start, "conversion to 'nullptr_t' from non-null value");
        // C11 6.5.4p2: a cast's type name must specify void or a scalar
        // type, and (unless the target is void) the operand must also
        // have scalar type -- struct/union values are never castable to
        // or from anything but void, with one GNU extension: a cast to a
        // union type is allowed when the operand's type matches (is
        // compatible with) one of the union's own member types --
        // reinterpreting the scalar/aggregate bits as that member,
        // exactly like an implicit union-member store would. An
        // array-typed operand decays to a pointer before ever reaching a
        // cast (same as any other array use), so it's exempt outright,
        // not merely folded into the aggregate check. GCC's own
        // vector-cast extensions reinterpret between vector
        // representations of matching size (a distinct rule from either
        // of these), so `is_vector` structs are exempted on both sides;
        // `(void)expr` discarding an aggregate value is the other
        // universally-supported exception.
        // A cast to the SAME struct type as the operand is a GCC-
        // tolerated no-op identity cast (unlike casting to a genuinely
        // *different* struct type, or from/to any other aggregate,
        // which stays rejected below) -- e.g. tinycc/c-testsuite's own
        // `(struct S)w->t.s` where `w->t.s` already has type `struct
        // S`, a pattern real-world code writes when a macro or
        // generated table always wraps a value in a cast regardless of
        // whether it happens to already match.
        bool same_struct_cast = ty->kind == TY_STRUCT && !ty->is_vector &&
            lhs->ty && types_compatible_p(ty, lhs->ty);
        if (ty->kind != TY_VOID && !same_struct_cast) {
            if (ty->kind == TY_STRUCT && !ty->is_vector)
                error_tok(start, "conversion to non-scalar type requested");
            if (ty->kind == TY_UNION && !ty->is_vector) {
                bool member_match = false;
                for (Member *m = ty->members; m; m = m->next) {
                    if (types_compatible_p(m->ty, lhs->ty)) {
                        member_match = true;
                        break;
                    }
                }
                if (!member_match)
                    error_tok(start, "cast to union type from type not present in union");
            }
            if (lhs->ty && (lhs->ty->kind == TY_STRUCT || lhs->ty->kind == TY_UNION) &&
                !lhs->ty->is_vector)
                error_tok(start, "aggregate value used where a scalar was expected");
        }
        Node *node = new_unary(ND_CAST, lhs, start);
        // Casts produce rvalues; top-level qualifiers are discarded (C99 6.5.4p5).
        // e.g. (float const)expr has type float, not float const.
        if (ty->qual) {
            ty = copy_type(ty);
            ty->qual = 0;
        }
        if (parser_current_fn && ty->kind == TY_PTR) {
            Node *vla_pre = NULL;
            ty = vla_freeze_dims(ty, &vla_pre, start);
            if (vla_pre) {
                Node *fcopy = arena_alloc(sizeof(Node));
                *fcopy = *vla_pre;
                // The node variable is declared above as ND_CAST
                Node *tmp = new_node(ND_COMMA, start);
                tmp->lhs = fcopy;
                tmp->rhs = new_unary(ND_CAST, lhs, start);
                tmp->rhs->ty = ty;
                check_type(tmp);
                return tmp;
            }
        }
        node->ty = ty;
        return node;
    }
    return primary(rest, tok);
}

static Node *mul(Token **rest, Token *tok) {
    Node *node = unary(&tok, tok);
    for (;;) {
        Token *start = tok;
        if (equalc(tok, "*")) {
            node = vector_lower(new_binary(ND_MUL, node, unary(&tok, tok->next), start));
            continue;
        }
        if (equalc(tok, "/")) {
            node = vector_lower(new_binary(ND_DIV, node, unary(&tok, tok->next), start));
            continue;
        }
        if (equalc(tok, "%")) {
            node = vector_lower(new_binary(ND_MOD, node, unary(&tok, tok->next), start));
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);
    for (;;) {
        Token *start = tok;
        if (equalc(tok, "+")) {
            node = vector_lower(new_binary(ND_ADD, node, mul(&tok, tok->next), start));
            continue;
        }
        if (equalc(tok, "-")) {
            node = vector_lower(new_binary(ND_SUB, node, mul(&tok, tok->next), start));
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *shift(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);
    for (;;) {
        Token *start = tok;
        if (equalc(tok, "<<")) {
            node = vector_lower(new_binary(ND_SHL, node, add(&tok, tok->next), start));
            continue;
        }
        if (equalc(tok, ">>")) {
            node = vector_lower(new_binary(ND_SHR, node, add(&tok, tok->next), start));
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *relational(Token **rest, Token *tok) {
    Node *node = shift(&tok, tok);
    for (;;) {
        Token *start = tok;
        if (equalc(tok, "<")) {
            node = vector_lower(new_binary(ND_LT, node, shift(&tok, tok->next), start));
            continue;
        }
        if (equalc(tok, "<=")) {
            node = vector_lower(new_binary(ND_LE, node, shift(&tok, tok->next), start));
            continue;
        }
        if (equalc(tok, ">")) {
            node = vector_lower(new_binary(ND_LT, shift(&tok, tok->next), node, start));
            continue;
        }
        if (equalc(tok, ">=")) {
            node = vector_lower(new_binary(ND_LE, shift(&tok, tok->next), node, start));
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);
    for (;;) {
        Token *start = tok;
        if (equalc(tok, "==")) {
            node = vector_lower(new_binary(ND_EQ, node, relational(&tok, tok->next), start));
            continue;
        }
        if (equalc(tok, "!=")) {
            node = vector_lower(new_binary(ND_NE, node, relational(&tok, tok->next), start));
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *bitand(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    while (equalc(tok, "&")) {
        Token *start = tok;
        node = vector_lower(new_binary(ND_BITAND, node, equality(&tok, tok->next), start));
    }
    *rest = tok;
    return node;
}

static Node *bitxor(Token **rest, Token *tok) {
    Node *node = bitand(&tok, tok);
    while (equalc(tok, "^")) {
        Token *start = tok;
        node = vector_lower(new_binary(ND_BITXOR, node, bitand(&tok, tok->next), start));
    }
    *rest = tok;
    return node;
}

static Node *bitor(Token **rest, Token *tok) {
    Node *node = bitxor(&tok, tok);
    while (equalc(tok, "|")) {
        Token *start = tok;
        node = vector_lower(new_binary(ND_BITOR, node, bitxor(&tok, tok->next), start));
    }
    *rest = tok;
    return node;
}

static Node *logand(Token **rest, Token *tok) {
    Node *node = bitor(&tok, tok);
    while (equalc(tok, "&&"))
        node = new_binary(ND_LOGAND, node, bitor(&tok, tok->next), tok);
    *rest = tok;
    return node;
}

static Node *logor(Token **rest, Token *tok) {
    Node *node = logand(&tok, tok);
    while (equalc(tok, "||"))
        node = new_binary(ND_LOGOR, node, logand(&tok, tok->next), tok);
    *rest = tok;
    return node;
}

static Node *conditional(Token **rest, Token *tok) {
    Node *node = logor(&tok, tok);
    if (equalc(tok, "?")) {
        Token *qtok = tok;
        Node *cond = node;
        Node *then;
        tok = tok->next; // consume '?'
        if (equalc(tok, ":")) {
            // GNU extension: a ?: b  — omit then-expr, use cond as value
            then = cond;
            tok = tok->next; // skip ':'
        } else {
            then = expr(&tok, tok);
            tok = skip(tok, ":");
        }
        Node *els = conditional(&tok, tok);
        node = new_node(ND_COND, qtok);
        node->cond = cond;
        node->then = then;
        node->els = els;
    }
    *rest = tok;
    return node;
}

// Compound assignment: a op= b → (tmp=&a, *tmp = *tmp op b)
// Evaluates the LHS exactly once, preventing double side-effects like a[i++] |= 1.
// Falls back to ASSIGN(lhs, OP(lhs, rhs)) for bitfields (can't take address of bitfield)
// and for GCC global register variables (same reason: no address at all).
static Node *to_assign(Node *binary) {
    check_type(binary->lhs);
    Token *tok = binary->tok;
    Node *lhs = binary->lhs;
    // Bitfields and global register variables can't be addressed; the old
    // ASSIGN(lhs, OP(lhs, rhs)) is safe for both because neither the
    // member access nor a register read has any side effects in the lhs
    // path (unlike e.g. `a[i++] |= 1`, the double-evaluation this
    // rewrite exists to avoid).
    if ((lhs->kind == ND_MEMBER && lhs->member && lhs->member->bit_width > 0) ||
        (lhs->kind == ND_LVAR && lhs->var && lhs->var->is_global_reg))
        return new_binary(ND_ASSIGN, lhs, new_binary(binary->kind, lhs, binary->rhs, tok), tok);
    Type *lhs_ty = lhs->ty;
    if (lhs_ty->kind == TY_ARRAY || lhs_ty->kind == TY_VLA)
        lhs_ty = pointer_to(lhs_ty->base);
    LVar *var = new_var("", pointer_to(lhs_ty), true);
    Node *addr_lhs = new_unary(ND_ADDR, lhs, tok);
    addr_lhs->ty = pointer_to(lhs_ty);
    Node *expr1 = new_binary(ND_ASSIGN, new_var_node(var, tok), addr_lhs, tok);
    expr1->ty = var->ty;
    Node *deref_r = new_unary(ND_DEREF, new_var_node(var, tok), tok);
    deref_r->ty = lhs_ty;
    Node *deref_w = new_unary(ND_DEREF, new_var_node(var, tok), tok);
    deref_w->ty = lhs_ty;
    // Swap LHS and RHS so the RHS (with possible side effects) is evaluated
    // before the old LHS value is read. This matches GCC's behavior for
    // compound assignments like x[0] |= foo() where foo() modifies x[0].
    // Only safe for commutative operations.
    bool commutative = (binary->kind == ND_BITOR || binary->kind == ND_BITAND ||
                        binary->kind == ND_BITXOR || binary->kind == ND_ADD ||
                        binary->kind == ND_MUL);
    Node *op = commutative
        ? new_binary(binary->kind, binary->rhs, deref_r, tok)
        : new_binary(binary->kind, deref_r, binary->rhs, tok);
    op = vector_lower(op);
    Node *expr2 = new_binary(ND_ASSIGN, deref_w, op, tok);
    return new_binary(ND_COMMA, expr1, expr2, tok);
}

static Node *assign(Token **rest, Token *tok) {
    Node *node = conditional(&tok, tok);
    if (equalc(tok, "="))
        node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next), tok);
    else if (equalc(tok, "+="))
        node = to_assign(new_binary(ND_ADD, node, assign(&tok, tok->next), tok));
    else if (equalc(tok, "-="))
        node = to_assign(new_binary(ND_SUB, node, assign(&tok, tok->next), tok));
    else if (equalc(tok, "*="))
        node = to_assign(new_binary(ND_MUL, node, assign(&tok, tok->next), tok));
    else if (equalc(tok, "/="))
        node = to_assign(new_binary(ND_DIV, node, assign(&tok, tok->next), tok));
    else if (equalc(tok, "%="))
        node = to_assign(new_binary(ND_MOD, node, assign(&tok, tok->next), tok));
    else if (equalc(tok, "&="))
        node = to_assign(new_binary(ND_BITAND, node, assign(&tok, tok->next), tok));
    else if (equalc(tok, "|="))
        node = to_assign(new_binary(ND_BITOR, node, assign(&tok, tok->next), tok));
    else if (equalc(tok, "^="))
        node = to_assign(new_binary(ND_BITXOR, node, assign(&tok, tok->next), tok));
    else if (equalc(tok, "<<="))
        node = to_assign(new_binary(ND_SHL, node, assign(&tok, tok->next), tok));
    else if (equalc(tok, ">>="))
        node = to_assign(new_binary(ND_SHR, node, assign(&tok, tok->next), tok));
    *rest = tok;
    return node;
}

static Node *expr(Token **rest, Token *tok) {
    Node *node = assign(&tok, tok);
    while (equalc(tok, ","))
        node = new_binary(ND_COMMA, node, assign(&tok, tok->next), tok);
    *rest = tok;
    return node;
}

static LVar *parse_params(Token **rest, Token *tok, bool *is_variadic) {
    LVar head = {};
    LVar *cur = &head;
    int param_index = 0;

    *is_variadic = false;
    if (equalc(tok, "void") && equalc(tok->next, ")")) {
        *rest = tok->next;
        return NULL;
    }

    while (!equalc(tok, ")")) {
        if (cur != &head)
            tok = skip(tok, ",");
        if (equalc(tok, "...")) {
            *is_variadic = true;
            tok = tok->next;
            break;
        }

        VarAttr attr = {};
        Type *base = declspec(&tok, tok, &attr);
        char *name = NULL;
        Type *ty = declarator(&tok, tok, copy_type(base), &name, &attr);
        tok = skip_attributes(tok);

        if (!name)
            name = format("__param%d", param_index++);

        if (equalc(tok, "(")) {
            tok = tok->next;
            // Handle extra grouping parens: int ((int)) - outer ( consumed, tok = (int))
            bool stripped_extra = false;
            if (equalc(tok, "(") && (is_typename(tok->next) || equalc(tok->next, ")") || equalc(tok->next, "..."))) {
                stripped_extra = true;
                tok = tok->next;
            }
            bool dummy_variadic = false;
            LVar *nested_params = parse_params(&tok, tok, &dummy_variadic);
            tok = skip(tok, ")");
            if (stripped_extra)
                tok = skip(tok, ")");
            ty = func_type(ty);
            Type param_head = {};
            Type *pcur = &param_head;
            for (LVar *p = nested_params; p; p = p->param_next) {
                Type *pt = arena_alloc(sizeof(Type));
                *pt = *p->ty;
                pt->param_next = NULL;
                pcur->param_next = pt;
                pcur = pt;
            }
            ty->param_types = param_head.param_next;
            ty = pointer_to(ty);
        }

        if (ty->kind == TY_ARRAY)
            ty = decay_to_ptr(ty);

        LVar *var = new_var(name, ty, true);
        cur = cur->param_next = var;
    }

    *rest = tok;
    return head.param_next;
}

static void global_initializer(Token **rest, Token *tok, LVar *var) {
    // See in_global_var_init's own comment: set for the duration of this
    // whole call tree (including nested recursive calls for compound
    // literals) so the general expression-parser's compound-literal
    // handling can tell "genuinely parsing a static/global object's
    // initializer" apart from merely being at block depth 0.
    bool saved_in_global_var_init = in_global_var_init;
    in_global_var_init = true;
    global_initializer_impl(rest, tok, var);
    in_global_var_init = saved_in_global_var_init;
}

static void global_initializer_impl(Token **rest, Token *tok, LVar *var) {
    // C23 empty initializer `{}` — zero-initialize an object of any type.
    if (equalc(tok, "{") && equalc(tok->next, "}")) {
        var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
        var->init_size = var->ty->size;
        var->has_init = true;
        *rest = tok->next->next; // skip `{` `}`
        return;
    }

    // A scalar (non-array/struct/union) wrapped in braces: `int *p = { 0 };`
    // is a superfluous-but-legal single-element brace initializer. Peel the
    // braces and recurse so the type-specific dispatch below sees the bare
    // expression, not the leading `{`.
    if (equalc(tok, "{") && var->ty->kind != TY_ARRAY && var->ty->kind != TY_STRUCT &&
        var->ty->kind != TY_UNION) {
        tok = tok->next;
        global_initializer(&tok, tok, var);
        if (equalc(tok, ","))
            tok = tok->next;
        *rest = skip(tok, "}");
        return;
    }

    if (var->ty->kind == TY_ARRAY && var->ty->base->kind == TY_CHAR && tok->kind == TK_STR && (tok->string_literal_prefix == 0 || tok->string_literal_prefix == '8')) {
        var->init_data = tok->str;
        var->init_size = tok->len + 1; // include embedded NULs and the terminator
        var->has_init = true;
        *rest = tok->next;
        return;
    }

    // A parenthesized string-literal chain — `char s[] = ( "a" "b" )` — is
    // still a string literal (C11 6.7.9p14), e.g. diffutils'
    // C_ifdef_group_formats. Concatenate the inner strings and store the
    // bytes (embedded NULs included); skip past the closing paren.
    if (var->ty->kind == TY_ARRAY && var->ty->base->kind == TY_CHAR && equalc(tok, "(")) {
        Token *t = tok->next;
        Token *first = NULL, *last = NULL;
        int total = 0;
        for (; t && t->kind == TK_STR; t = t->next) {
            if (!first) first = t;
            last = t;
            total += t->len;
        }
        if (first && equalc(t, ")")) {
            int len = total + 1;
            char *buf = arena_alloc(len);
            int pos = 0;
            for (Token *u = first; u != last->next; u = u->next) {
                memcpy(buf + pos, u->str, u->len);
                pos += u->len;
            }
            buf[len - 1] = 0;
            var->init_data = buf;
            var->init_size = len;
            var->has_init = true;
            *rest = t->next;
            return;
        }
    }

    // codeql[cpp/commented-out-code]: doc comment naming the wide-string prefixes handled below, not dead code
    // Wide string literal L"..."/u"..."/U"..." for wchar_t/char16_t/char32_t array
    if (var->ty->kind == TY_ARRAY && tok->kind == TK_STR &&
        (tok->string_literal_prefix == 'L' || tok->string_literal_prefix == 'u' ||
         tok->string_literal_prefix == 'U') &&
        (var->ty->base->size == 4 || var->ty->base->size == 2)) {
        // Count UTF-8 codepoints to size the array
        char *p = tok->str;
        char *end = p + tok->len;
        int count = 0;
        while (p < end) {
            char *np = p;
            decode_utf8(&np, p);
            p = np;
            count++;
        }
        count++; // null terminator
        if (var->ty->size == 0)
            var->ty = array_of(var->ty->base, count);
        var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
        var->init_size = var->ty->size;
        var->has_init = true;
        global_init_one(tok, var, var->ty, 0);
        *rest = tok->next;
        return;
    }

    // A string literal reaching here (array target, neither the narrow
    // char/char8_t branch above nor the wide 2/4-byte-element branch
    // matched) has a prefix that's incompatible with the target array's
    // element type — e.g. u8"..." (itself an array of unsigned char, per
    // the standard) assigned to a char16_t/char32_t/wchar_t array, or an
    // L/u/U-prefixed literal assigned to a plain char/char8_t array. This
    // is a real constraint violation in C; without this check it silently
    // fell through to the generic (address-only) initializer fallback
    // below, which never validates element-type/width compatibility at
    // all — no diagnostic, wrong bytes.
    if (var->ty->kind == TY_ARRAY && tok->kind == TK_STR &&
        var->ty->base->kind != TY_ARRAY && var->ty->base->kind != TY_STRUCT &&
        var->ty->base->kind != TY_UNION) {
        error_tok(tok, "initializing an array of incompatible element type "
                       "with a string literal");
    }

    if (var->ty->kind == TY_PTR) {
        char *label = NULL;
        int addend = 0;
        if (read_global_label_initializer(&tok, tok, &label, &addend)) {
            var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
            var->init_size = var->ty->size;
            var->has_init = true;
            append_reloc(var, 0, label, addend);
            *rest = tok;
            return;
        }
        // Pointer initialized with &(compound literal): &(struct T){...}
        if (equalc(tok, "&") && find_compound_literal_start(tok->next)) {
            tok = tok->next; // skip &
            // C23: file-scope compound literals may not specify register/thread_local
            Token *tt = tok;
            while (equalc(tt, "(")) tt = tt->next;
            for (Token *u = tt; u && !equalc(u, ")"); u = u->next)
                if (equalc(u, "register") || equalc(u, "thread_local") || equalc(u, "_Thread_local"))
                    error_tok(u, "file-scope compound literal specifies storage class");

            Token *compound_start = find_compound_literal_start(tok);
            Token *t = tok;
            int open_count = 0;
            while (equalc(t, "(")) {
                t = t->next;
                open_count++;
            }
            bool saved_icl = in_compound_literal;
            in_compound_literal = true;
            Type *compound_ty = type_name(&t, t);
            in_compound_literal = saved_icl;
            int close_count = 0;
            while (equalc(t, ")")) {
                t = t->next;
                close_count++;
            }
            static int anon_count;
            char *name = format(".Lanon.%d", anon_count++);
            LVar *anon_var = new_var(name, compound_ty, false);
            anon_var->is_static = true; // see the identical comment above
            global_initializer(rest, compound_start, anon_var);
            tok = *rest;
            // Redundant parens wrapped directly around the compound
            // literal itself close here, immediately after its body.
            for (int i = 0; i < open_count - close_count; i++)
                tok = skip(tok, ")");
            // A trailing chain of constant subscripts/member accesses may
            // still follow — e.g. "&(...)[0].attr.attr" — fold it into the
            // relocation's addend instead of leaving it unparsed.
            int chain_addend = parse_const_addend_chain(&tok, tok, compound_ty);
            while (equalc(tok, ")"))
                tok = tok->next;
            var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
            var->init_size = var->ty->size;
            var->has_init = true;
            append_reloc(var, 0, name, chain_addend);
            *rest = tok;
            return;
        }
        // Pointer initialized with compound literal (array-to-pointer decay):
        // int *p = (int [3]) { 1, 2, 3 };
        if (find_compound_literal_start(tok)) {
            Token *compound_start = find_compound_literal_start(tok);
            Token *t = tok;
            while (equalc(t, "(")) t = t->next;
            // Skip storage class specifiers (static, constexpr, etc.)
            while (kw_is(t, KW_STORAGE)) t = t->next;
            Type *compound_ty = type_name(&t, t);
            while (equalc(t, ")")) t = t->next;
            // The compound literal may be wrapped in a cast:
            // "(T)(T2{...})". The anonymous object must get the *literal's*
            // type T2, not the cast target T -- so skip the cast layer and
            // parse the inner type (e.g. Cello's `(var)((var[]){...})`).
            while (equalc(t, "(")) {
                // Skip ALL parens, not just one: "(T)(((T2){...}))" nests
                // the literal's own paren around the type's parens, so a
                // single t = t->next would land on the type's '(' and make
                // type_name() default to int.
                while (equalc(t, "(")) t = t->next;
                while (kw_is(t, KW_STORAGE)) t = t->next;
                compound_ty = type_name(&t, t);
                while (equalc(t, ")")) t = t->next;
            }
            static int anon_count2;
            // Distinct prefix from the other anon counters: this TU may
            // also emit ".Lanon.N" objects from element-level
            // "&(compound literal)" initializers (same file), and two
            // local symbols sharing one name coalesce into one object.
            char *name = format(".Lanoncast.%d", anon_count2++);
            LVar *anon_var = new_var(name, compound_ty, false);
            anon_var->is_static = true; // see the identical comment above
            global_initializer(rest, compound_start, anon_var);
            tok = *rest;
            if (equalc(tok, "}"))
                tok = tok->next;
            while (equalc(tok, ")"))
                tok = tok->next;
            // A constant pointer-addend after the literal, e.g. Cello's
            // CelloObject(): "(char*)((var[]){ ... }) + sizeof(struct
            // Header)". Fold it into the relocation offset; the enclosing
            // cast's trailing ")" is consumed below.
            long long addend = 0;
            if (equalc(tok, "+")) {
                tok = tok->next;
                Node *add_node = assign(&tok, tok);
                check_type(add_node);
                eval_const_expr(add_node, &addend);
            }
            while (equalc(tok, ")"))
                tok = tok->next;
            var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
            var->init_size = var->ty->size;
            var->has_init = true;
            append_reloc(var, 0, name, (int)addend);
            *rest = tok;
            return;
        }
        // Try parsing as expression and extracting reloc
        Node *node = assign(&tok, tok);
        check_type(node);
        if (extract_reloc(node, &label, &addend)) {
            var->has_init = true;
            if (label) {
                var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
                var->init_size = var->ty->size;
                append_reloc(var, 0, label, addend);
            } else {
                var->init_val = addend;
            }
            *rest = tok;
            return;
        }
        // Full-width constant scalar (e.g. `(void*)0xdeadbeef`, a value that
        // overflows extract_reloc()'s int addend): fall back to the 64-bit
        // const-expr evaluator so the value isn't truncated/sign-extended.
        {
            long long v = 0;
            if (eval_const_expr(node, &v)) {
                var->has_init = true;
                var->init_val = v;
                *rest = tok;
                return;
            }
        }
        if (!var->is_local && !in_speculative_const_fold)
            error_tok(tok, "unsupported global initializer");
        else if (in_speculative_const_fold)
            speculative_fold_failed = true;
        return;
    }

    if (var->ty->kind == TY_ARRAY && equalc(tok, "{")) {
        int len = array_len(var->ty);
        if (len == 0) {
            Token *tmp = tok;
            len = count_array_initializer(&tmp, tmp, var->ty->base);
            var->ty = array_of(var->ty->base, len);
        }
        var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
        var->init_size = var->ty->size;
        var->has_init = true;
        *rest = global_init_one(tok, var, var->ty, 0);
        return;
    }

    if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION) && equalc(tok, "{")) {
        var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
        var->init_size = var->ty->size;
        var->has_init = true;
        tok = global_init_one(tok, var, var->ty, 0);
        *rest = tok;
        return;
    }

    // Struct/union initialized with a compound literal: (Type){...} or ((Type){...})
    if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION) && find_compound_literal_start(tok)) {
        var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
        var->init_size = var->ty->size;
        var->has_init = true;
        tok = global_init_one(tok, var, var->ty, 0);
        *rest = tok;
        return;
    }

    // Struct/union initialized by copy from another already-initialized
    // global/constexpr variable of the same type (`constexpr struct s v = other;`):
    // copy its init_data bytes so member accesses on `var` remain constant-foldable.
    if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION) && tok->kind == TK_IDENT &&
        (equalc(tok->next, ";") || equalc(tok->next, ","))) {
        LVar *src = find_global_name(tok->name);
        if (src && src->has_init && src->init_data && (src->ty == var->ty || (src->ty->kind == var->ty->kind && src->ty->size == var->ty->size))) {
            int sz = var->ty->size ? var->ty->size : 1;
            var->init_data = arena_alloc(sz);
            memcpy(var->init_data, src->init_data, sz);
            var->init_size = var->ty->size;
            var->has_init = true;
            *rest = tok->next;
            return;
        }
    }

    // Compound literal for scalar type: delegate to inner initializer
    if (find_compound_literal_start(tok)) {
        global_initializer(rest, find_compound_literal_start(tok), var);
        tok = *rest;
        while (equalc(tok, ")")) tok = tok->next;
        *rest = tok;
        return;
    }
    // Scalar with braces: superfluous `{ expr }` or C23 empty init `{}`.
    if (equalc(tok, "{")) {
        tok = skip(tok, "{");
        if (equalc(tok, "}")) {
            // C23 `= {}`: zero-initialize.
            var->has_init = true;
            var->init_val = 0;
            *rest = skip(tok, "}");
            return;
        }
        global_initializer(&tok, tok, var);
        *rest = skip(tok, "}");
        return;
    }

    // Try to parse as an expression and evaluate
    {
        Node *node = assign(&tok, tok);
        check_type(node);
        // If the initializer is a constexpr struct/union with init_data, copy bytes
        if (node->kind == ND_LVAR && node->var && node->var->is_constexpr && node->var->init_data) {
            int sz = var->ty->size > 0 ? var->ty->size : node->var->ty->size;
            var->init_data = arena_alloc(sz);
            var->init_size = sz;
            var->has_init = true;
            memcpy(var->init_data, node->var->init_data, sz);
            *rest = tok;
            return;
        }

        // Try float constant evaluation for float types
        if (is_flonum(var->ty) || (node->ty && is_flonum(node->ty))) {
            double fv = 0;
            if (eval_double_const_expr(node, &fv)) {
                int sz = var->ty->size ? var->ty->size : 8;
                var->has_init = true;
                var->init_data = arena_alloc(sz);
                var->init_size = sz;
                if (sz == 4) {
                    float f = (float)fv;
                    memcpy(var->init_data, &f, 4);
                } else {
                    memcpy(var->init_data, &fv, 8);
                }
                *rest = tok;
                return;
            }
        }
        // Try complex constant evaluation
        if (is_complex(var->ty)) {
            double rv = 0.0, iv = 0.0;
            if (eval_complex_const_expr(node, &rv, &iv)) {
                int base_sz = var->ty->base ? var->ty->base->size : 8;
                int sz = var->ty->size ? var->ty->size : base_sz * 2;
                var->has_init = true;
                var->init_data = arena_alloc(sz);
                var->init_size = sz;
                if (base_sz == 4) {
                    float rf = (float)rv, imf = (float)iv;
                    memcpy(var->init_data, &rf, 4);
                    memcpy(var->init_data + 4, &imf, 4);
                } else {
                    // Imag part sits at base_sz (8 for double, 16 for long
                    // double whose slot holds a double payload).
                    memcpy(var->init_data, &rv, 8);
                    memcpy(var->init_data + base_sz, &iv, 8);
                }
                *rest = tok;
                return;
            }
        }


        // _Decimal32/64/128: the literal was folded to BID bits at parse
        // time (node->val low word, node->val2 high word for decimal128).
        // Write the raw bits into init_data (all sizes; decimal128 needs
        // both words — init_val alone would truncate it to 8 bytes). An
        // integer constant initializer (e.g. `_Decimal64 d = 3;`) is
        // converted through the BID runtime's int->decimal helpers.
        if (is_decimal(var->ty)) {
            int sz = var->ty->size;
            unsigned long long lo = 0, hi = 0;
            if (node && node->kind == ND_NUM && is_decimal(node->ty)) {
                lo = (unsigned long long)node->val;
                hi = (unsigned long long)node->val2;
            } else {
#ifndef __MUSL__
                // int/float constant -> decimal via the linked libbid.
                long long iv = 0;
                if (eval_const_expr(node, &iv)) {
                    if (var->ty->kind == TY_DECIMAL128) {
                        BID_UINT128 r = __bid64_to_bid128(__bid64_from_int64(iv));
                        lo = r.w[0];
                        hi = r.w[1];
                    } else if (var->ty->kind == TY_DECIMAL64) {
                        lo = __bid64_from_int64(iv);
                    } else {
                        lo = (unsigned long long)__bid64_to_bid32(__bid64_from_int64(iv));
                    }
                }
#endif
            }
            var->has_init = true;
            var->init_data = arena_alloc(sz);
            var->init_size = sz;
            memcpy(var->init_data, &lo, sz <= 8 ? sz : 8);
            if (sz > 8) {
                memcpy(var->init_data + 8, &hi, 8);
            }
            // Also keep init_val (the BID bits as a 64-bit pattern) so
            // constant-folding paths that read var->init_val for a
            // constexpr decimal global (e.g. `e == 7.0dd` folding) agree
            // with the emitted data. decimal128's high word is not
            // representable; callers must use init_data for those.
            var->init_val = (int64_t)lo;
            *rest = tok;
            return;
        }

        // A relocatable address stored in an integer-typed (not pointer-
        // typed) scalar — e.g. arch/x86/include/asm/processor.h's
        // INIT_THREAD: "{ .sp = (unsigned long)&__top_init_kernel_stack }",
        // where .sp is a plain `unsigned long`, not a pointer. Not a
        // foldable constant (eval_const_expr above correctly rejects it —
        // the address isn't known until link time) but not an error
        // either: the TY_PTR branch above already handles exactly this
        // shape via extract_reloc()/append_reloc() for pointer-typed
        // globals; scalars just never tried the same fallback.
        //
        // Must run BEFORE the plain eval_const_expr() below: that
        // evaluator folds a string literal (or any address expression) to
        // its truthiness (ND_STR -> 1), never its address, so
        // `(intptr_t)"lit"` / `(unsigned long)&sym` in an integer scalar
        // would silently store 1 instead of a relocation (git's `struct
        // option` tables, `.defval = (intptr_t)"all"`).
        //
        // Only fires when the field is at least pointer-width — see the
        // matching guard in global_init_one() for why: on LLP64 (Windows)
        // "unsigned long" is 4 bytes, too narrow to guarantee a real address
        // fits, and GCC itself rejects the cast there as non-constant.
        if (var->ty->size >= 8) {
            char *label = NULL;
            int addend = 0;
            if (looks_like_address_expr(node) && extract_reloc(node, &label, &addend) && label) {
                var->has_init = true;
                var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
                var->init_size = var->ty->size;
                append_reloc(var, 0, label, addend);
                *rest = tok;
                return;
            }
        }

        // Try integer constant evaluation
        long long ival = 0;
        if (eval_const_expr(node, &ival)) {
            var->has_init = true;
            var->init_val = (int64_t)ival;
            *rest = tok;
            return;
        }

        if (is_decimal(var->ty) && node && node->kind == ND_NUM) {
            int sz = var->ty->size;
            var->has_init = true;
            var->init_data = arena_alloc(sz);
            var->init_size = sz;
            unsigned long long lo = (unsigned long long)node->val;
            memcpy(var->init_data, &lo, sz <= 8 ? sz : 8);
            if (sz > 8) {
                unsigned long long hi = (unsigned long long)node->val2;
                memcpy(var->init_data + 8, &hi, 8);
            }
            *rest = tok;
            return;
        }

        // For float comparisons stored in int (e.g. static int e1 = -1.0 == 0.0)
        {
            double fv = 0;
            if (eval_double_const_expr(node, &fv)) {
                var->has_init = true;
                var->init_val = (int64_t)fv;
                *rest = tok;
                return;
            }
        }

        // A pointer/address expression stored in a _Bool global, e.g.
        // `bool e = &s;` or `bool has_handler = my_func;` (function-name
        // decay). C11 6.3.1.2: converting a pointer to _Bool yields 1 if
        // the pointer is non-null, 0 otherwise -- &x and a function name
        // are never null, so this always folds to the constant 1, with
        // no need for the actual (link-time) address value at all.
        if (var->ty->kind == TY_BOOL && looks_like_address_expr(node)) {
            var->has_init = true;
            var->init_val = 1;
            *rest = tok;
            return;
        }

        if (!var->is_local && !in_speculative_const_fold)
            error_tok(tok, "unsupported global initializer");
        else if (in_speculative_const_fold)
            speculative_fold_failed = true;
    }
}

static char *parse_toplevel_asm(Token **rest, Token *tok) {
    while (equalc(tok, "volatile") || equalc(tok, "__volatile__") ||
           equalc(tok, "__volatile") || equalc(tok, "goto"))
        tok = tok->next;
    tok = skip(tok, "(");
    if (tok->kind != TK_STR)
        error_tok(tok, "expected string literal in asm");
    char buf[4096];
    int pos = 0;
    while (tok->kind == TK_STR) {
        int n = tok->len;
        if (pos + n < (int)sizeof(buf)) {
            memcpy(buf + pos, tok->str, n);
            pos += n;
        }
        tok = tok->next;
    }
    buf[pos] = '\0';
    // Skip operand sections (outputs, inputs, clobbers, goto labels)
    while (!equalc(tok, ")")) {
        tok = skip(tok, ":");
        while (!equalc(tok, ":") && !equalc(tok, ")")) {
            if (equalc(tok, "[")) {
                tok = tok->next;
                if (tok->kind == TK_IDENT) tok = tok->next;
                if (equalc(tok, "]")) tok = tok->next;
                continue;
            }
            if (tok->kind == TK_STR) tok = tok->next;
            if (equalc(tok, "(")) {
                int depth = 1;
                tok = tok->next;
                while (depth > 0 && tok->kind != TK_EOF) {
                    if (equalc(tok, ")")) depth--;
                    else if (equalc(tok, "("))
                        depth++;
                    tok = tok->next;
                }
            }
            if (equalc(tok, ",")) tok = tok->next;
        }
    }
    tok = skip(tok, ")");
    *rest = tok;
    return str_intern(buf, pos);
}

// Error recovery (GH #34): on a recoverable parse error, error_tok() longjmps
// back into parse()'s top-level loop. State that must survive the longjmp
// lives in these statics (locals of parse() would be indeterminate).
static TLItem tl_item_head;
static TLItem *tl_item_cur;
static Token *rec_iter_tok;
static TypedefLog *rec_typedef_cp;
static TagLog *rec_tag_cp;
static EnumLog *rec_enum_cp;
static Typedef *rec_typedefs;
static TagScope *rec_tags;
static EnumConst *rec_enum_consts;

// Skip tokens to the next top-level synchronization point: a ';' at brace
// depth 0 or the '}' closing the outermost open block (plus a trailing ';').
// `depth` is the compound-statement nesting depth at the error site.
static Token *sync_toplevel(Token *tok, int depth) {
    while (tok->kind != TK_EOF) {
        if (equalc(tok, "{")) {
            depth++;
        } else if (equalc(tok, "}")) {
            depth--;
            if (depth <= 0) {
                tok = tok->next;
                if (equalc(tok, ";"))
                    tok = tok->next;
                return tok;
            }
        } else if (equalc(tok, ";") && depth <= 0) {
            return tok->next;
        }
        tok = tok->next;
    }
    return tok;
}

Program *parse(Token *tok) {
    static char *kw_main;
    static bool parser_inited = false;
    if (!parser_inited) {
        kw_main = str_intern("main", 4);
        parser_inited = true;
    }

    char *saved_input = current_input;
    char *saved_filename = current_filename;
    char *saved_debug_filename = current_debug_filename;
    int saved_line_offset = current_line_offset;
    int saved_line_num = line_num;
    Token *head = tokenize("rcc_builtins",
#if defined(ARCH_ARM64) && defined(__APPLE__)
                           "typedef unsigned char char8_t;"
                           // Apple ARM64: va_list is char* (simple pointer ABI)
                           "typedef char *__builtin_va_list;"
                           // Declare libc builtins with correct return types
                           "void *memcpy(void *, const void *, unsigned long);"
                           "void *memmove(void *, const void *, unsigned long);"
                           "void *memset(void *, int, unsigned long);"
                           "int memcmp(const void *, const void *, unsigned long);"
                           "unsigned long strlen(const char *);"
                           "char *strcpy(char *, const char *);"
                           "char *strncpy(char *, const char *, unsigned long);"
                           "int strcmp(const char *, const char *);"
                           "int strncmp(const char *, const char *, unsigned long);"
                           "char *strchr(const char *, int);"
                           "char *strrchr(const char *, int);"
                           "void *malloc(unsigned long);"
                           "void *calloc(unsigned long, unsigned long);"
                           "void *realloc(void *, unsigned long);"
                           "void free(void *);"
#elif defined(ARCH_ARM64)
                           // AArch64 AAPCS64 va_list: 32 bytes
                           "typedef unsigned char char8_t;"
                           "typedef struct {"
                           "  void *__stack;"
                           "  void *__gr_top;"
                           "  void *__vr_top;"
                           "  int __gr_offs;"
                           "  int __vr_offs;"
                           "} __builtin_va_list[1];"
#elif defined(_WIN32)
                           // Windows x64: va_list is just a char pointer (msvcrt ABI)
                           "typedef unsigned char char8_t;"
                           "typedef char *__builtin_va_list;"
#else
                           "typedef unsigned char char8_t;"
                           // x86-64 System V ABI va_list: 24 bytes
                           "typedef struct {"
                           "  unsigned int gp_offset;"
                           "  unsigned int fp_offset;"
                           "  void *overflow_arg_area;"
                           "  void *reg_save_area;"
                           "} __builtin_va_list[1];"
#endif
    );
    current_input = saved_input;
    current_debug_filename = saved_debug_filename;
    current_filename = saved_filename;
    current_line_offset = saved_line_offset;
    line_num = saved_line_num;
    Token *t = head;
    while (t->next && t->next->kind != TK_EOF)
        t = t->next;
    t->next = tok;
    tok = head;

    globals = NULL;
    parser_used_wide_bitint = false;
    parser_used_decimal = false;
    memset(global_htab, 0, sizeof(global_htab));
    memset(typedef_htab, 0, sizeof(typedef_htab));
    typedef_log = NULL;
    memset(tag_htab, 0, sizeof(tag_htab));
    tag_log = NULL;
    memset(enum_htab, 0, sizeof(enum_htab));
    enum_log = NULL;
    str_lits = NULL;
    tl_item_head.next = NULL;
    tl_item_cur = &tl_item_head;

    // Recovery point: error_tok() longjmps here on a parse error (unless
    // -Wfatal-errors). Restore file-scope parser state and resynchronize
    // at the next top-level ';' or matching '}'.
    error_recovery_tok = NULL;
    stmt_recovery_active = false;
    if (setjmp(error_recovery_jmp)) {
        // All compound-statement frames died in the unwind.
        stmt_recovery_active = false;
        current_fn_is_inline = false;
        int depth = current_block_depth;
        locals = NULL;
        label_scopes = NULL;
        pending_gotos = NULL;
        current_switch = NULL;
        current_loop = NULL;
        parser_current_fn = NULL;
        current_fn_scope_locals = NULL;
        current_block_depth = 0;
        suppress_fn_scope_update = false;
        fn_uses_vla = false;
        pending_asm_name = NULL;
        pending_alias_target = NULL;
        pending_section_name = NULL;
        pending_constructor = false;
        pending_destructor = false;
        pending_cleanup_func = NULL;
        pending_cleanup_tok = NULL;
        pending_vla_struct_capture = NULL;
        pending_mode = 0;
        pending_vector_size = 0;
        pending_transparent_union = false;
        pending_weak = false;
        pending_visibility_set = false;
        pending_target_attr = NULL;
        if (pending_target_clones) {
            free(pending_target_clones);
            pending_target_clones = NULL;
        }
        pending_target_clones_n = 0;
        typedef_scope_restore(rec_typedef_cp);
        tag_scope_restore(rec_tag_cp);
        enum_scope_restore(rec_enum_cp);
        typedefs = rec_typedefs;
        tags = rec_tags;
        enum_consts = rec_enum_consts;
        tok = sync_toplevel(error_recovery_tok, depth);
        if (tok == rec_iter_tok) // no forward progress: force a skip
            tok = sync_toplevel(tok->next, 0);
    }
    error_recovery_active = true;

    while (tok->kind != TK_EOF) {
        // Checkpoint file-scope state for error recovery.
        rec_iter_tok = tok;
        rec_typedef_cp = typedef_scope_checkpoint();
        rec_tag_cp = tag_scope_checkpoint();
        rec_enum_cp = enum_scope_checkpoint();
        rec_typedefs = typedefs;
        rec_tags = tags;
        rec_enum_consts = enum_consts;

        if (equalc(tok, "#") && equalc(tok->next, "pragma") &&
            equalc(tok->next->next, "pack")) {
            tok = tok->next->next->next; // skip '# pragma pack'
            if (equalc(tok, "(")) {
                tok = tok->next;
                if (tok->kind == TK_NUM)
                    pack_align = tok->val;
                else
                    pack_align = 0;
                tok = tok->next;
                if (equalc(tok, ")"))
                    tok = tok->next;
            }
            continue;
        }

        if (equalc(tok, "#") && equalc(tok->next, "pragma") &&
            equalc(tok->next->next, "fenv")) {
            tok = tok->next->next->next;
            if (equalc(tok, "(")) {
                tok = tok->next;
                if (tok->kind == TK_NUM)
                    fenv_access = tok->val;
                else
                    fenv_access = false;
                tok = tok->next;
                if (equalc(tok, ")"))
                    tok = tok->next;
            }
            continue;
        }
        if (tok->kw == ID_ASM || tok->kw == ID___ASM || tok->kw == ID___ASM__) {
            tok = tok->next;
            char *str = parse_toplevel_asm(&tok, tok);
            TLItem *item = arena_alloc(sizeof(TLItem));
            item->kind = TL_ASM;
            item->asm_str = str;
            tl_item_cur = tl_item_cur->next = item;
            tok = skip(tok, ";");
            continue;
        }

        if (equalc(tok, ";")) {
            tok = tok->next;
            continue;
        }

        // _Pragma("string") — C99 pragma operator at file scope. Treat as a
        // no-op; a following ';' is an (empty) declaration, not a type-less one,
        // so consume it here instead of falling through to declaration parsing
        // (which would wrongly warn "type defaults to int").
        if (tok->kw == ID__PRAGMA || equalc(tok, "_Pragma")) {
            tok = tok->next;
            tok = skip(tok, "(");
            if (tok->kind == TK_STR)
                tok = tok->next;
            tok = skip(tok, ")");
            while (equalc(tok, ";"))
                tok = tok->next;
            continue;
        }

        // C23 attribute-declaration at file scope: `[[...]];` is a standalone
        // no-op (not attributes on a following declaration). Only consume when
        // the attribute list is immediately followed by ';'; otherwise leave it
        // for the declaration parser (declspec handles leading attributes).
        if (equalc(tok, "[") && equalc(tok->next, "[") &&
            tok->ptr + tok->len == tok->next->ptr) {
            Token *after_attr = skip_attributes(tok);
            if (equalc(after_attr, ";")) {
                tok = after_attr->next;
                continue;
            }
        }


        if (equalc(tok, "_Static_assert") || equalc(tok, "static_assert")) {
            Token *st = tok;
            tok = skip(tok->next, "(");
            Node *cond = conditional(&tok, tok);
            check_type(cond);
            if (opt_pedantic && cond->ty && !is_integer(cond->ty))
                warn_tok(cond->tok, "static_assert condition is not an integer");
            // C11 6.6p6: floating operands only as immediate cast operands
            if (cond->kind == ND_CAST && cond->lhs && cond->lhs->ty &&
                is_flonum(cond->lhs->ty) && cond->lhs->kind != ND_FNUM)
                warn_tok(cond->tok,
                         "static_assert condition is not an integer constant expression");
            long long v = 0;
            if (!eval_const_expr(cond, &v))
                error_tok(cond->tok, "static_assert condition must be constant");
            char *msg = "static_assert failed";
            if (equalc(tok, ",")) {
                tok = tok->next;
                if (tok->kind == TK_STR) {
                    msg = tok->str;
                    tok = tok->next;
                } else {
                    error_tok(tok, "expected string literal in static assertion");
                }
            }
            tok = skip(tok, ")");
            tok = skip(tok, ";");
            if (!v) error_tok(st, "%s", msg);
            continue;
        }
        VarAttr attr = {};
        Type *base = declspec(&tok, tok, &attr);

        if (equalc(tok, ";")) {
            // C11 6.7.4p2: _Noreturn requires a function declarator
            if (attr.has_alignas)
                error_tok(tok, "alignment specified for unnamed declaration");
            if (attr.has_alignas)
                error_tok(tok, "alignment specified for unnamed declaration");
            if (attr.is_noreturn_std)
                error_tok(tok, "'_Noreturn' in empty declaration");
            if (attr.is_auto || attr.is_auto_type) {
                if (attr.is_auto_type && !(attr.is_extern || attr.is_static || attr.is_typedef))
                    error_tok(tok, "empty declaration");
                else if (attr.is_auto)
                    error_tok(tok, "'auto' in empty declaration");
            }
            if (base->is_enum_fixed) {
                if (attr.is_extern || attr.is_static)
                    error_tok(tok, "storage class specifier in empty declaration with 'enum' underlying type");
                if (attr.is_tls)
                    error_tok(tok, "'_Thread_local' in empty declaration with 'enum' underlying type");
                if (attr.is_inline)
                    error_tok(tok, "'inline' in empty declaration");
                if (attr.is_noreturn_std)
                    error_tok(tok, "'_Noreturn' in empty declaration");
                if (attr.is_auto || attr.is_auto_type)
                    error_tok(tok, "'auto' in file-scope empty declaration");
                if (attr.is_register)
                    error_tok(tok, "'register' in file-scope empty declaration");
                if (attr.has_alignas)
                    error_tok(tok, "'alignas' in empty declaration with 'enum' underlying type");
            }
            // GCC: a type qualifier on a tag-only declaration with no
            // object (`const struct S;`) qualifies nothing -- the tag
            // type itself, not any instance of it.
            if ((base->kind == TY_STRUCT || base->kind == TY_UNION || base->is_enum) && base->qual)
                warn_tok(tok, "useless type qualifier");
            tok = tok->next;
            continue;
        }

        for (;;) {
            EnumConst *top_enum_consts_cp = enum_consts;
            EnumLog *top_enum_log_cp = enum_scope_checkpoint();
            int top_decl_align = 0;
            char *name = NULL;
            Token *decl_start = tok;
            Type *ty = declarator(&tok, tok, copy_type(base), &name, &attr);
            tok = read_type_attrs(tok, &top_decl_align, &attr);
            // Transfer C23 function type attributes from VarAttr to the Type
            if (attr.is_reproducible || attr.is_unsequenced) {
                Type *fty = ty;
                if (fty->kind == TY_PTR && fty->base && fty->base->kind == TY_FUNC)
                    fty = fty->base;
                if (fty->kind == TY_FUNC) {
                    fty = copy_type(fty);
                    fty->is_reproducible = attr.is_reproducible;
                    fty->is_unsequenced = attr.is_unsequenced;
                    if (ty->kind == TY_PTR) {
                        ty = copy_type(ty);
                        ty->base = fty;
                    } else {
                        ty = fty;
                    }
                }
            }
            // GCC __attribute__((__transparent_union__)) trails the
            // declarator (typically a typedef name): `typedef union {...}
            // name __attribute__((transparent_union));`. declarator()
            // consumes that trailing attribute itself (it's called with
            // attr=NULL from most sites, see pending_transparent_union's
            // comment) and stashes it here rather than in the local `attr`.
            // Mark the union Type itself so a function argument matching
            // one of its members later skips the (bogus, boxing-implying)
            // implicit cast to the union in check_type's ND_FUNCALL handling.
            if (pending_transparent_union && ty->kind == TY_UNION)
                ty->is_transparent_union = true;
            pending_transparent_union = false;

            if (!name) {
                enum_scope_restore(top_enum_log_cp);
                enum_consts = top_enum_consts_cp;
                tok = skip(tok, ";");
                break;
            }

            bool is_func = ty->kind == TY_FUNC || equalc(tok, "(");

            // C11 6.7.1: _Thread_local cannot appear with typedef or on
            // a function declaration.
            if (attr.has_alignas && (attr.is_typedef || is_func))
                error_tok(tok, attr.is_typedef ? "alignment specified for typedef" : "alignment specified for function");
            if (attr.has_alignas && (attr.is_typedef || is_func))
                error_tok(tok, attr.is_typedef ? "alignment specified for typedef" : "alignment specified for function");
            if (attr.is_tls && (attr.is_typedef || is_func))
                error_tok(tok, attr.is_typedef ? "'_Thread_local' with typedef" : "'_Thread_local' storage class on function");

            if (is_func) {
                // Apply trailing attribute alignment to function type
                if (top_decl_align > 0 && ty->kind == TY_FUNC) {
                    ty = copy_type(ty);
                    ty->align = top_decl_align;
                }
                Type *fty;
                bool is_variadic = false;
                LVar *params = NULL;
                fn_uses_vla = false;

                if (ty->kind == TY_FUNC) {
                    fty = ty;
                    is_variadic = ty->is_variadic;
                    locals = NULL;
                    stack_offset = CHAIN_RSP_OFFSET; // reserve [80,96) uniformly: any top-level function's __label__s may be a nonlocal-goto target - see codegen.c's prologue, which unconditionally records its own stable sp there
                    LVar param_head = {};
                    LVar *cur = &param_head;
                    int param_index = 0;
                    for (Type *pt = ty->param_types; pt; pt = pt->param_next) {
                        char *pname = pt->name ? pt->name : format("__param%d", param_index++);
                        LVar *lvar;
                        if (pt->vla_len_val) {
                            // Reuse placeholder LVar from declarator_params; update offset
                            // so VLA dim expressions (e.g. a++) reference the correct slot.
                            lvar = (LVar *)pt->vla_len_val;
                            // Struct/union types must keep pointer identity
                            // with the typedef table's own Type* for
                            // type_equal() (_Generic, __builtin_types_
                            // compatible_p) — pt is a shallow `*pt = *pty`
                            // clone (see declarator_params) and would break
                            // that; lvar->ty already correctly holds the
                            // original (set in declarator_params, preserved
                            // through this reuse). Every other kind (VLA
                            // params decay to a pointer before reaching
                            // here, so this is never struct/union for them)
                            // still re-syncs to pt as before.
                            if (pt->kind != TY_STRUCT && pt->kind != TY_UNION)
                                lvar->ty = pt;
                            int sz = pt->size < 4 ? 4 : pt->size;
                            int al = pt->align < 4 ? 4 : pt->align;
                            stack_offset = align_to(stack_offset + sz, al);
                            lvar->offset = stack_offset;
                            lvar->next = locals;
                            locals = lvar;
                            lvar->param_next = NULL;
                        } else {
                            lvar = new_var(pname, pt, true);
                        }
                        cur = cur->param_next = lvar;
                    }
                    params = param_head.param_next;
                } else {
                    fty = func_type(ty);
                    bool is_oldstyle = false;
                    locals = NULL;
                    stack_offset = CHAIN_RSP_OFFSET; // reserve [80,96) uniformly - see codegen.c's prologue
                    label_scopes = NULL;
                    pending_gotos = NULL;
                    current_switch = NULL;
                    current_loop = NULL;
                    parser_current_fn = name;

                    tok = tok->next;
                    if (!equalc(tok, ")") && !equalc(tok, "...") && !is_typename(tok)) {
                        // K&R function definition: param list has identifiers, not types
                        is_oldstyle = true;
                        KRParam *kr_list = parse_kr_param_list(&tok, tok);
                        // Second pass: create LVars with correct types and offsets
                        LVar param_head = {};
                        LVar *cur = &param_head;
                        for (KRParam *krp = kr_list; krp; krp = krp->next) {
                            if (!krp->ty)
                                krp->ty = ty_int;
                            LVar *var = new_var(krp->name, krp->ty, true);
                            if (krp->vla_len_expr)
                                var->ty->vla_len_expr = krp->vla_len_expr; // fn-entry emission loop picks it up
                            cur = cur->param_next = var;
                        }
                        params = param_head.param_next;
                        current_fn_scope_locals = params;
                    } else {
                        params = parse_params(&tok, tok, &is_variadic);
                        tok = skip(tok, ")");
                        tok = skip_attributes(tok);
                        current_fn_scope_locals = params;
                    }

                    // Build parameter type list
                    fty->is_variadic = is_variadic;
                    fty->is_oldstyle = is_oldstyle;
                    Type param_head = {};
                    Type *pcur = &param_head;
                    if (fty->return_ty && (fty->return_ty->kind == TY_STRUCT || fty->return_ty->kind == TY_UNION || fty->return_ty->kind == TY_COMPLEX || (fty->return_ty->kind == TY_BITINT && fty->return_ty->size > 16))) {
                        Type *pt = arena_alloc(sizeof(Type));
                        *pt = *pointer_to(fty->return_ty);
                        pt->param_next = NULL;
                        pcur = pcur->param_next = pt;
                    }
                    for (LVar *p = params; p; p = p->param_next) {
                        Type *pt = arena_alloc(sizeof(Type));
                        *pt = *p->ty;
                        pt->param_next = NULL;
                        pcur = pcur->param_next = pt;
                    }
                    fty->param_types = param_head.param_next;
                }

                label_scopes = NULL;
                pending_gotos = NULL;
                current_switch = NULL;
                current_loop = NULL;
                parser_current_fn = name;
                current_fn_scope_locals = params;
                current_block_depth = 0;
                bool was_oldstyle = fty->is_oldstyle;

                // Preserve alignment from prior declaration
                if (!top_decl_align) {
                    LVar *prev = find_global_name(name);
                    if (prev && prev->ty && prev->ty->base) {
                        if (prev->ty->base->align > fty->align)
                            fty->align = prev->ty->base->align;
                        if (prev->ty->base->param_types)
                            fty->is_oldstyle = false;
                    }
                }

                if (fty->return_ty && (fty->return_ty->kind == TY_STRUCT || fty->return_ty->kind == TY_UNION || fty->return_ty->kind == TY_COMPLEX || (fty->return_ty->kind == TY_BITINT && fty->return_ty->size > 16))) {
                    LVar *retbuf = new_var("__retbuf", pointer_to(fty->return_ty), true);
                    retbuf->cleanup_func = NULL;
                }

                // For typedefs like 'typedef int functype(int);', register the type
                if (attr.is_typedef) {
                    // C11 6.7.4p2: _Noreturn only on function declarations
                    if (attr.is_noreturn_std)
                        error_tok(tok, "'_Noreturn' with typedef");
                    add_typedef(name, fty);
                    if (!equalc(tok, ";") && !equalc(tok, ","))
                        error_tok(tok, "expected ';' or ',' after typedef");
                } else {
                    // Register function symbol
                    Type *fn_symbol_ty = pointer_to(fty);
                    LVar *existing = find_global_name(name);
                    LVar *fn_lvar = existing;
                    if (!existing) {
                        fn_lvar = new_var(name, fn_symbol_ty, false);
                        fn_lvar->is_synthetic_prelude = rec_iter_tok && rec_iter_tok->filename &&
                            !strcmp(rec_iter_tok->filename, "rcc_builtins");
                        fn_lvar->is_extern = attr.is_extern || (!attr.is_inline && !attr.is_static);
                        fn_lvar->is_function = true;
                        fn_lvar->is_inline = attr.is_inline;
                        fn_lvar->is_weak = attr.is_weak;
                        fn_lvar->has_visibility = attr.has_visibility;
                        fn_lvar->visibility = attr.visibility;
                        fn_lvar->is_reproducible = attr.is_reproducible;
                        fn_lvar->is_unsequenced = attr.is_unsequenced;
                        fn_lvar->is_static = attr.is_static;
                        if (attr.diag_warning) fn_lvar->diag_warning = attr.diag_warning;
                        if (attr.diag_entries) fn_lvar->diag_entries = attr.diag_entries;
                        if (attr.diag_error) fn_lvar->diag_error = attr.diag_error;
                        if (pending_asm_name)
                            fn_lvar->asm_name = pending_asm_name;
                        if (pending_alias_target) {
                            fn_lvar->alias_target = pending_alias_target;
                            pending_alias_target = NULL;
                        }
                        // A declaration without inline (and without static) makes the
                        // function an external symbol even if a prior inline def exists.
                        if (!attr.is_inline && !attr.is_static)
                            fn_lvar->has_init = true; // reuse has_init as "has non-inline decl"
                    } else {
                        if (!existing->is_synthetic_prelude &&
                            !(attr.is_extern && attr.is_inline) &&
                            existing->ty && existing->ty->base && !was_oldstyle) {
                            if (func_decls_conflict(existing->ty->base, fty))
                                error_tok(tok, "conflicting types for '%s'", name);
                        }
                        existing->ty = fn_symbol_ty;
                        // Update flags on redeclaration
                        if (attr.is_inline)
                            existing->is_inline = true;
                        if (attr.is_weak)
                            existing->is_weak = true;
                        if (attr.is_static)
                            existing->is_static = true;
                        if (attr.is_extern)
                            existing->is_extern = true;
                        if (pending_asm_name)
                            existing->asm_name = pending_asm_name;
                        if (pending_alias_target) {
                            existing->alias_target = pending_alias_target;
                            pending_alias_target = NULL;
                        }
                        if (!attr.is_inline && !attr.is_static)
                            existing->has_init = true; // non-inline extern decl seen
                    }
                }

                if (equalc(tok, "{")) {
                    if (attr.is_typedef)
                        error_tok(tok, "typedef cannot have function body");
                    // C11 6.9p3: at most one external definition per
                    // identifier in this TU (a function's own symbol is
                    // always registered above by this point, prototype
                    // or not). Point at the declarator's own identifier
                    // token (matching gcc's "int f(void)" column), not
                    // the '{' that merely triggered this check.
                    LVar *prior_def = find_global_name(name);
                    if (prior_def && prior_def->has_definition) {
                        Token *name_tok = tok;
                        for (Token *t = decl_start; t && t != tok; t = t->next)
                            if (t->kind == TK_IDENT && t->name == name) {
                                name_tok = t;
                                break;
                            }
                        error_tok(name_tok, "redefinition of '%s'", name);
                    }
                    if (prior_def)
                        prior_def->has_definition = true;

                    LVar *fn_locals = NULL;
                    // A statement-level declaration inside the body
                    // (declaration() resets the pending attribute flags so
                    // they can't leak onto the NEXT top-level declaration)
                    // would otherwise wipe the constructor/destructor
                    // attributes attached to THIS definition before they
                    // are consumed below -- `static void
                    // __attribute__((constructor)) f(void) { int x; }`
                    // silently lost its .init_array entry (found via
                    // gnutls' lib_init: the ASN.1 tree was never built at
                    // load, and DH params imported before
                    // gnutls_global_init() failed). Capture them before
                    // parsing the body.
                    bool def_is_constructor = pending_constructor;
                    bool def_is_destructor = pending_destructor;
                    char *def_asm_name = pending_asm_name;
                    char *def_alias_target = pending_alias_target;
                    // extern inline IS the external definition, right
                    // here where any static object it references is
                    // visible -- gcc never warns there (only for a bare
                    // `inline` fn, which might be emitted as *the*
                    // external definition in some other TU instead).
                    current_fn_is_inline = attr.is_inline && !attr.is_static &&
                        !attr.is_gnu_inline && !attr.is_extern;
                    Node *contract_pre_checks = activate_function_contracts(fty, name, params, tok);
                    Node *body = compound_stmt_ex(&tok, tok, &fn_locals, false);
                    current_fn_is_inline = false;
                    // Implicit return 0 for main if no explicit return
                    if (name == kw_main) {
                        Node *last = body->body;
                        if (last) {
                            while (last->next)
                                last = last->next;
                            if (!ends_in_return(last)) {
                                Node *ret = new_node(ND_RETURN, tok);
                                ret->lhs = new_num(0, tok);
                                last->next = apply_postconds_to_return(ret, tok);
                            }
                        } else {
                            // Empty body: insert return 0
                            Node *ret = new_node(ND_RETURN, tok);
                            ret->lhs = new_num(0, tok);
                            body->body = apply_postconds_to_return(ret, tok);
                        }
                    } else if (fty->return_ty && fty->return_ty->kind == TY_VOID && current_fn_postconds) {
                        // Postconditions on a void function must also be
                        // checked at the implicit end-of-body exit (a
                        // fallthrough past the closing '}' with no
                        // explicit `return;` — see
                        // apply_postconds_to_return()).
                        Node *last = body->body;
                        if (!ends_in_return(last)) {
                            Node *implicit_ret = new_node(ND_RETURN, tok);
                            Node *wrapped = apply_postconds_to_return(implicit_ret, tok);
                            if (last) {
                                while (last->next)
                                    last = last->next;
                                last->next = wrapped;
                            } else {
                                body->body = wrapped;
                            }
                        }
                    }
                    current_fn_postconds = NULL;
                    current_fn_postcond_binds = NULL;
                    Function *fn = arena_alloc(sizeof(Function));
                    fn->name = name;
                    LVar *fn_sym2 = find_global_name(name);
                    fn->asm_name = def_asm_name ? def_asm_name
                                                : (fn_sym2 ? fn_sym2->asm_name : NULL);
                    fn->alias_target = def_alias_target;
                    fn->ty = fty;
                    fn->params = params;
                    fn->locals = fn_locals;
                    fn->body = body->body;
                    // Prepend VLA parameter dimension side-effect expressions.
                    // e.g. void foo(int a, int b[a++]) must increment a at entry.
                    {
                        Node **ins = &fn->body;
                        for (Type *pt = fty->param_types; pt; pt = pt->param_next) {
                            if (pt->vla_len_expr) {
                                check_type(pt->vla_len_expr);
                                Node *s = new_unary(ND_EXPR_STMT, pt->vla_len_expr,
                                                    pt->vla_len_expr->tok);
                                s->next = *ins;
                                *ins = s;
                                ins = &s->next;
                            }
                        }
                    }
                    // Precondition checks run before everything else,
                    // including the VLA parameter side effects just above.
                    if (contract_pre_checks) {
                        Node *tail = contract_pre_checks;
                        while (tail->next)
                            tail = tail->next;
                        tail->next = fn->body;
                        fn->body = contract_pre_checks;
                    }
                    fn->stack_size = align_to(stack_offset, 16);
                    fn->is_variadic = is_variadic;
                    fn->dealloc_vla = fn_uses_vla;
                    fn->is_constructor = def_is_constructor ||
                        (fn_sym2 && fn_sym2->is_constructor);
                    fn->is_destructor = def_is_destructor ||
                        (fn_sym2 && fn_sym2->is_destructor);
                    fn->is_inline = attr.is_inline;
                    fn->is_gnu_inline = attr.is_gnu_inline;
                    fn->is_always_inline = attr.is_always_inline;
                    // is_static is sticky: if any decl was static the fn is static
                    fn->is_static = attr.is_static || (fn_sym2 && fn_sym2->is_static);
                    // is_extern: explicit extern on this def, OR any non-inline extern
                    // declaration seen (has_init flag).
                    fn->is_extern = attr.is_extern || (fn_sym2 && fn_sym2->has_init);
                    fn->is_weak = attr.is_weak || pending_weak || (fn_sym2 && fn_sym2->is_weak);
                    fn->has_visibility = attr.has_visibility || pending_visibility_set ||
                        (fn_sym2 && fn_sym2->has_visibility);
                    fn->visibility = attr.has_visibility ? attr.visibility
                                                         : (pending_visibility_set ? pending_visibility
                                                                                   : (fn_sym2 ? fn_sym2->visibility : STV_DEFAULT));
                    fn->is_used = attr.is_used || (fn_sym2 && fn_sym2->is_used);
                    pending_constructor = false;
                    pending_destructor = false;
                    pending_weak = false;
                    pending_visibility_set = false;
                    pending_asm_name = NULL;
                    pending_alias_target = NULL;
                    pending_section_name = NULL;
                    fn->target_clones = pending_target_clones;
                    fn->n_target_clones = pending_target_clones_n;
                    fn->target_attr = pending_target_attr;
                    pending_target_clones = NULL;
                    pending_target_clones_n = 0;
                    pending_target_attr = NULL;
                    // "extern inline __attribute__((always_inline, gnu_inline))"
                    // variadic functions using __builtin_va_arg_pack() are never
                    // emitted; each call site is expanded inline instead (see
                    // inline_pack_call).
                    if (fn->is_variadic && node_uses_va_arg_pack(fn->body)) {
                        InlinePackFn *ipf = arena_alloc(sizeof(InlinePackFn));
                        ipf->name = fn->name;
                        ipf->fn = fn;
                        ipf->next = inline_pack_fns;
                        inline_pack_fns = ipf;
                    } else {
                        TLItem *item = arena_alloc(sizeof(TLItem));
                        item->kind = TL_FUNC;
                        item->fn = fn;
                        tl_item_cur = tl_item_cur->next = item;
                    }
                    // Clear `locals`: it still holds this function's
                    // params/locals (fn->locals already captured the
                    // real list via compound_stmt_ex's fn_locals out
                    // param above). Left dangling, the next top-level
                    // item — if not itself a function definition, which
                    // is the only other site that resets `locals` — sees
                    // this function's stale symbols in find_var()'s
                    // locals-before-globals lookup order (same bug as
                    // the prototype-only ";" case below, just triggered
                    // by a function *definition* instead).
                    enum_scope_restore(top_enum_log_cp);
                    enum_consts = top_enum_consts_cp;
                    locals = NULL;
                    current_fn_scope_locals = NULL;
                    current_block_depth = 0;
                    suppress_fn_scope_update = false;
                    parser_current_fn = NULL;
                    break;
                }

                if (equalc(tok, ";")) {
                    pending_asm_name = NULL;
                    pending_alias_target = NULL;
                    pending_section_name = NULL;
                    tok = tok->next;
                    // Prototype only (no body): undo the tentative
                    // function-scope state set up above (parser_current_fn,
                    // current_fn_scope_locals, ...) for a possible
                    // definition — this declarator turned out to be just a
                    // declaration. Leaving parser_current_fn dangling here
                    // means the *next* top-level global (e.g. a static
                    // array right after `static T f(int x);`) inherits this
                    // function's name as its LVar.decl_fn_name, so if `f`
                    // is later found unused and DCE-omitted, opt.c's
                    // "drop globals owned by an omitted function" pass
                    // wrongly drops that unrelated global too (e.g. rcc
                    // demoted a live `static const char *const arr[]` used
                    // by other functions to an undefined extern symbol).
                    // Also clear `locals` itself (not just the
                    // current_fn_scope_locals snapshot): parse_params()/
                    // declarator_params() populate `locals` with each
                    // parameter's LVar as a side effect while tentatively
                    // preparing for a possible definition. Left in place
                    // for a prototype-only declarator, a parameter name
                    // (e.g. a single-letter `f`) permanently shadows any
                    // later file-scope identifier of the same name in
                    // find_var()'s locals-before-globals lookup order —
                    // e.g. a subsequent `enum K { ..., f, ... }`'s `f`
                    // silently resolved to this phantom parameter instead
                    // of the enum constant.
                    enum_scope_restore(top_enum_log_cp);
                    enum_consts = top_enum_consts_cp;
                    locals = NULL;
                    current_fn_scope_locals = NULL;
                    current_block_depth = 0;
                    suppress_fn_scope_update = false;
                    parser_current_fn = NULL;
                    // A prototype-only declaration consumed the pending
                    // constructor/destructor attributes: record them on the
                    // symbol so the later definition inherits them, and clear
                    // the pending globals so the NEXT declaration (e.g. a
                    // destructor right after a constructor) can't stack both
                    // flags onto one function.
                    if (pending_constructor || pending_destructor) {
                        LVar *fn_sym = find_global_name(name);
                        if (fn_sym) {
                            if (pending_constructor) fn_sym->is_constructor = true;
                            if (pending_destructor) fn_sym->is_destructor = true;
                        }
                    }
                    pending_constructor = false;
                    pending_destructor = false;
                    break;
                }
                if (equalc(tok, ",")) {
                    enum_scope_restore(top_enum_log_cp);
                    enum_consts = top_enum_consts_cp;
                    if (pending_constructor || pending_destructor) {
                        LVar *fn_sym = find_global_name(name);
                        if (fn_sym) {
                            if (pending_constructor) fn_sym->is_constructor = true;
                            if (pending_destructor) fn_sym->is_destructor = true;
                        }
                    }
                    pending_constructor = false;
                    pending_destructor = false;
                    tok = tok->next;
                    continue;
                }
                // Consume GCC function specifiers like __cond_acquires(true, lock)
                while (tok->kind == TK_IDENT && tok->next && equalc(tok->next, "(")) {
                    tok = tok->next;
                    tok = skip(tok, "(");
                    int pdepth = 1;
                    while (pdepth > 0 && tok->kind != TK_EOF) {
                        if (equalc(tok, "(")) pdepth++;
                        else if (equalc(tok, ")"))
                            pdepth--;
                        tok = tok->next;
                    }
                    if (equalc(tok, ","))
                        tok = tok->next;
                }
                if (equalc(tok, ";") || equalc(tok, "{") || equalc(tok, ",")) {
                    if (pending_constructor || pending_destructor) {
                        LVar *fn_sym = find_global_name(name);
                        if (fn_sym) {
                            if (pending_constructor) fn_sym->is_constructor = true;
                            if (pending_destructor) fn_sym->is_destructor = true;
                        }
                    }
                    pending_constructor = false;
                    pending_destructor = false;
                    enum_scope_restore(top_enum_log_cp);
                    enum_consts = top_enum_consts_cp;
                    break;
                }
                error_tok(tok, "expected ';', ',', or '{'");
            } else {
                // C11 6.7.4p2: _Noreturn only on function declarations
                if (attr.is_auto_type) {
                    if (ty->kind == TY_PTR)
                        error_tok(tok, "plain identifier with auto");
                    if (!equalc(tok, "="))
                        error_tok(tok, "initialized data declaration");
                }
                if (attr.is_auto && !attr.is_auto_type)
                    error_tok(tok, "file-scope declaration");
                if (attr.is_noreturn_std)
                    error_tok(tok, "'_Noreturn' on a non-function declaration");
                if (attr.is_typedef) {
                    add_typedef(name, ty);
                } else {
                    LVar *var = find_global_name(name);
                    bool var_is_new = false;
                    if (!var) {
                        // Register EARLY (before infer_array_type()'s own
                        // initializer skip/count pass below), so a
                        // self-referential initializer resolves --
                        // e.g. mquickjs's/njs's ROM-table idiom
                        // "static const T arr[] = { ..., &arr[N], ... };",
                        // referencing the array being defined from within
                        // its own initializer. Legal per C11 6.2.1p7 (an
                        // identifier's scope begins right after its
                        // declarator completes, well before the
                        // initializer is parsed), but
                        // count_array_initializer()/skip_initializer()
                        // below still call assign() on each element to
                        // skip it correctly -- which resolves `arr` via
                        // the ordinary identifier lookup and previously
                        // hard-errored "undeclared variable" since `var`
                        // wasn't registered until after this whole
                        // initializer-sizing pass ran.
                        var = new_var(name, ty, false);
                        var_is_new = true;
                    }
                    // C23 `constexpr auto x = other;`: infer x's type from a
                    // simple identifier initializer naming another global,
                    // since declspec() left `ty` as the ty_int placeholder.
                    if (attr.is_auto_type && equalc(tok, "=") && tok->next &&
                        tok->next->kind == TK_IDENT &&
                        (equalc(tok->next->next, ";") || equalc(tok->next->next, ","))) {
                        LVar *src = find_global_name(tok->next->name);
                        if (src)
                            ty = src->ty;
                    }
                    if (equalc(tok, "="))
                        ty = infer_array_type(ty, tok->next);
                    // C11 6.2.2: thread-local must agree across declarations
                    if (!var_is_new && !var->is_function && var->is_tls != attr.is_tls)
                        error_tok(tok, "'%s' redeclared with different thread-local storage",
                                  name);
                    if (!var_is_new && var->ty->kind == TY_ARRAY && ty->kind == TY_ARRAY && var->ty->size > 0)
                        ty = var->ty;
                    else
                        var->ty = ty;
                    // A redeclaration of an already-defined global (either an
                    // initialized definition, or a prior non-extern tentative
                    // definition like `int i;`) must not drop its definition
                    // or change its linkage: e.g. `int i = 1; extern int i;`
                    // keeps i defined, `int i; extern int i;` keeps i defined
                    // (C11 6.9.2), and `static auto u = 10U; extern unsigned
                    // u;` keeps u static. Only a genuinely new name, or one
                    // still `extern`-only so far, may pick up this
                    // declaration's own `extern`-ness.
                    if (!var->has_init) {
                        if (var_is_new || var->is_extern)
                            var->is_extern = attr.is_extern;
                        var->is_static = attr.is_static;
                        var->is_tls = attr.is_tls;
                        // __attribute__((weak)) may appear either as a
                        // prefix specifier (attr.is_weak, from
                        // declspec()) or trailing the declarator itself
                        // (pending_weak, set by declarator() -- see its
                        // declaration above; previously only consumed by
                        // the function-definition path below, leaving a
                        // plain global variable's own trailing
                        // `__attribute__((weak))` silently dropped, e.g.
                        // the Plan9/Go-toolchain AUTOLIB() idiom
                        // `int __p9l_autolib_x __attribute__((weak));`).
                        var->is_weak = attr.is_weak || pending_weak;
                        if (attr.has_visibility || pending_visibility_set) {
                            var->has_visibility = true;
                            var->visibility = attr.has_visibility ? attr.visibility : pending_visibility;
                        }
                    }
                    pending_weak = false;
                    pending_visibility_set = false;
                    if (attr.is_register && pending_asm_name && !var->has_init &&
                        ty->size > 0 && ty->size <= 8 &&
                        ty->kind != TY_STRUCT && ty->kind != TY_UNION && ty->kind != TY_ARRAY) {
                        // GCC "global register variable" extension: no storage, no
                        // symbol — every reference reads the named hardware register
                        // directly (see codegen.c's ND_LVAR/gen_addr handling for
                        // is_global_reg). Do NOT also set var->asm_name: that field
                        // renames a real linker symbol, and this variable has none —
                        // every TU that includes the declaring header (e.g. asm/asm.h's
                        // `register unsigned long current_stack_pointer asm("rsp");`)
                        // would otherwise emit its own colliding non-static "rsp"
                        // tentative definition in .bss, producing real link-time
                        // "multiple definition of `rsp`" errors (seen building the
                        // x86-64 vDSO, whose vclock_gettime.o/vgetcpu.o/vgetrandom.o
                        // all include it).
                        var->is_register = true;
                        var->is_global_reg = true;
                        var->global_reg_name = pending_asm_name;
                    } else if (pending_asm_name) {
                        var->asm_name = pending_asm_name;
                    }
                    if (pending_alias_target)
                        var->alias_target = pending_alias_target;
                    if (pending_section_name)
                        var->section_name = pending_section_name;
                    pending_asm_name = NULL;
                    pending_alias_target = NULL;
                    pending_section_name = NULL;
                    if (equalc(tok, "=")) {
                        // C23 6.7.1p6: a 'constexpr' object of integer type
                        // needs an integer constant expression initializer;
                        // a floating-typed expression (5.0, 2*2.5, i.x) is not.
                        if (attr.is_constexpr && is_integer(ty) &&
                            !equalc(tok->next, "{")) {
                            Token *probe_tok = tok->next;
                            Node *probe = assign(&probe_tok, probe_tok);
                            check_type(probe);
                            if (probe->ty && !is_integer(probe->ty))
                                error_tok(tok->next,
                                          "'constexpr' integer initializer is not "
                                          "an integer constant expression");
                        }
                        tok = tok->next;
                        bool saved_ici3 = in_constexpr_init;
                        if (attr.is_constexpr)
                            in_constexpr_init = true;
                        global_initializer(&tok, tok, var);
                        in_constexpr_init = saved_ici3;
                    }
                    if (attr.is_constexpr) {
                        var->is_constexpr = true;
                        // C23 6.2.2: constexpr at file scope gives internal linkage
                        var->is_static = true;
                        // constexpr implies const
                        var->ty = qualify_type_copy(ty, QUAL_CONST);
                        if (!var->has_init)
                            error("constexpr variable must be initialized");
                    }
                }

                if (equalc(tok, ";")) {
                    enum_scope_restore(top_enum_log_cp);
                    enum_consts = top_enum_consts_cp;
                    tok = tok->next;
                    break;
                }
                if (equalc(tok, ",")) {
                    enum_scope_restore(top_enum_log_cp);
                    enum_consts = top_enum_consts_cp;
                    if (attr.is_auto_type || attr.is_auto)
                        error_tok(tok, "only a single declarator allowed with `auto`");
                    tok = tok->next;
                    continue;
                }
                error_tok(tok, "expected ';' or ','");
            }
        }
    }

    error_recovery_active = false;

    Program *prog = arena_alloc(sizeof(Program));
    prog->items = tl_item_head.next;
    prog->globals = globals;
    prog->strs = str_lits;
    return prog;
}
