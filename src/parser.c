// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from chibicc by Rui Ueyama.
#include "rcc.h"

typedef struct VarAttr VarAttr;
typedef struct TagScope TagScope;
typedef struct EnumConst EnumConst;

struct VarAttr {
    bool is_typedef;
    bool is_extern;
    bool is_static;
    bool is_inline;
    bool is_gnu_inline;
    bool is_weak;
    bool is_tls;
    bool has_type;
    bool is_packed;
    bool is_constexpr;
    bool is_auto_type;
    char *diag_warning;
    char *diag_error;
    DiagEntry *diag_entries;
    unsigned char bitfield_mode;
    bool is_noreturn;
    bool is_deprecated;
    char *deprecated_msg;
    bool is_reproducible;
    bool is_unsequenced;
};

struct TagScope {
    TagScope *next;
    TagScope *hash_next;
    char *name;
    Type *ty;
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

static uint32_t hash_name(const char *s) {
    uint32_t h = 2166136261u;
    for (int i = 0; s[i]; i++) {
        h ^= (unsigned char)s[i];
        h *= 16777619;
    }
    return h;
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
static Token *pending_cleanup_tok;
static bool pending_constructor;
static bool pending_destructor;
static int pending_mode; // 0=none, 1=QI, 2=HI, 3=SI, 4=DI
static int pending_vector_size; // GCC __attribute__((vector_size(N))): total bytes, 0=none
static char *pending_asm_name;
static char *pending_alias_target;
// VLA-containing struct: emit size-capture code before the next statement
static Node *pending_vla_struct_capture;

static StrLit *str_lits;
static int str_lit_counter;

static Node *current_switch;
static Node *current_loop;
static int static_local_counter;
static LVar *current_fn_scope_locals;
static char *parser_current_fn;
static int current_block_depth;
static bool suppress_fn_scope_update;
static bool fn_uses_vla;

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
    Type *pt = fty->param_types;
    for (Node **arg = &call->args; *arg && pt; arg = &(*arg)->next, pt = pt->param_next) {
        check_type(*arg);
        if (!(*arg)->ty)
            continue;
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

static Node *new_num(int64_t val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    // Determine type from suffix encoded in token text
    char *end = tok->ptr + tok->len;
    bool is_u = false;
    int l_count = 0;
    for (char *s = end - 1; s >= tok->ptr; s--) {
        char c = *s;
        if (c == 'u' || c == 'U') {
            is_u = true;
        } else if (c == 'l' || c == 'L') {
            l_count++;
        } else
            break;
    }
    if (l_count >= 2)
        node->ty = is_u ? ty_ullong : ty_llong;
    else if (l_count == 1)
        node->ty = is_u ? ty_ulong : ty_long;
    else if (is_u)
        node->ty = (val >= 0 && val <= 0xFFFFFFFFLL) ? ty_uint : ty_ullong;
    else if (val >= -2147483648LL && val <= 2147483647LL)
        node->ty = ty_int;
    else if (tok->ptr[0] == '0' && tok->len > 1 && val >= 0 && val <= 4294967295LL)
        node->ty = ty_uint;
    else
        node->ty = ty_llong;
    return node;
}

static Node *new_fnum(double fval, Token *tok) {
    Node *node = new_node(ND_FNUM, tok);
    node->fval = fval;
    node->ty = tok->val == 2 ? ty_ldouble : ty_double;
    return node;
}

// Compute the byte size expression for a VLA allocation: count * element_size
static Node *vla_alloc_size(Type *ty, Token *tok) {
    Node *base_sz = (ty->base->kind == TY_VLA)
        ? vla_alloc_size(ty->base, tok)
        : new_node(ND_NUM, tok);
    if (ty->base->kind != TY_VLA) {
        base_sz->val = ty->base->size;
        base_sz->ty = ty_ulong;
    }
    Node *count = ty->vla_len_expr ? ty->vla_len_expr : new_node(ND_NUM, tok);
    if (!ty->vla_len_expr) {
        count->val = ty->array_len;
        count->ty = ty_ulong;
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
    if (align <= 0 || align <= ty->align)
        return ty;
    Type *ret = copy_type(ty);
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

static LVar *find_var(Token *tok) {
    for (LVar *var = locals; var; var = var->next)
        if (var->name == tok->name)
            return var;
    return find_global_name(tok->name);
}

LVar *find_global_name(char *name) {
    uint32_t h = hash_name(name) % GLOBAL_HASH_SIZE;
    for (LVar *var = global_htab[h]; var; var = var->hash_next)
        if (var->name == name)
            return var;
    return NULL;
}

// Does this AST (sub)tree contain a __builtin_va_arg_pack() placeholder?
static bool node_uses_va_arg_pack(Node *n) {
    if (!n)
        return false;
    if (n->kind == ND_VA_ARG_PACK)
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
    c->body = clone_inline_stmts(n->body, ctx);
    c->args = clone_inline_args(n->args, ctx);
    c->stmt_expr_result = clone_inline_node(n->stmt_expr_result, ctx);
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
    for (LVar *var = begin; var && var != end; var = var->next)
        if (var->is_local && var->cleanup_func)
            cur = cur->next = make_cleanup_stmt(var, tok);
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
        if (var->is_local && var->cleanup_func)
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
    if (tok->kw == ID___ATTRIBUTE || tok->kw == ID___ATTRIBUTE__ ||
        tok->kw == ID___DECLSPEC || tok->kw == ID__ALIGNAS)
        return true;
    tok = skip_attributes(tok);
    if (kw_is(tok, KW_TYPE | KW_QUAL | KW_STORAGE))
        return true;
    return find_typedef(tok) != NULL;
}

static Type *declspec(Token **rest, Token *tok, VarAttr *attr);
static Node *expr(Token **rest, Token *tok);
bool eval_const_expr(Node *node, long long *val);
static bool eval_const_addr_expr(Node *node, long long *val);
static void global_initializer(Token **rest, Token *tok, LVar *var);

static void maybe_update_align(int *align, int value) {
    if (align && value > *align)
        *align = value;
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
            tok = tok->next->next; // skip [[
            while (tok && tok->kind != TK_EOF &&
                   !(equalc(tok, "]") && tok->next && equalc(tok->next, "]") && tok->ptr + tok->len == tok->next->ptr)) {
                if (equalc(tok, ",")) {
                    tok = tok->next;
                    continue;
                }
                // namespace:: (two separate : tokens)
                if (tok->next && equalc(tok->next, ":") &&
                    tok->next->next && equalc(tok->next->next, ":")) {
                    tok = tok->next->next->next; // skip ident ::
                    continue;
                }
                bool consumed = false;
                if (tok->kind != TK_IDENT)
                    error_tok(tok, "expected attribute identifier");
                if (attr) {
                    if (equalc(tok, "noreturn"))
                        attr->is_noreturn = true;
                    else if (equalc(tok, "deprecated")) {
                        attr->is_deprecated = true;
                        Token *next = tok->next;
                        if (equalc(next, "(") && next->next && next->next->kind == TK_STR) {
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
            continue;
        }
        if (tok->kw == ID__ALIGNAS ||
            (tok->kw == ID_ALIGNAS && opt_std_version && strcmp(opt_std_version, "202311L") >= 0)) {
            tok = tok->next;
            tok = skip(tok, "(");
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

                if (equalc(tok, "weak")) {
                    if (attr)
                        attr->is_weak = true;
                    tok = tok->next;
                    if (equalc(tok, "("))
                        tok = skip_balanced(tok);
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
                        tok = tok->next;
                    }
                    tok = skip(tok, ")");
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

        if (tok->kw == ID___DECLSPEC) {
            tok = tok->next;
            if (equalc(tok, "("))
                tok = skip_balanced(tok);
            continue;
        }

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

bool eval_const_expr(Node *node, long long *val) {
    long long lhs;
    long long rhs;

    if (!node)
        return false;

    switch (node->kind) {
    case ND_NUM:
        *val = node->val;
        return true;
    case ND_FNUM:
        *val = (long long)node->fval;
        return true;
    case ND_ADD:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs + rhs), true);
    case ND_SUB:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs - rhs), true);
    case ND_MUL:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs * rhs), true);
    case ND_DIV:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && rhs != 0 && ((*val = rhs == -1 ? -lhs : lhs / rhs), true);
    case ND_MOD:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && rhs != 0 && ((*val = rhs == -1 ? 0 : lhs % rhs), true);
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
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs == rhs), true);
    case ND_NE:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs != rhs), true);
    case ND_LT:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = (node->lhs->ty && node->lhs->ty->is_unsigned) || (node->rhs->ty && node->rhs->ty->is_unsigned) ? (unsigned long long)lhs < (unsigned long long)rhs : lhs < rhs), true);
    case ND_LE:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = (node->lhs->ty && node->lhs->ty->is_unsigned) || (node->rhs->ty && node->rhs->ty->is_unsigned) ? (unsigned long long)lhs <= (unsigned long long)rhs : lhs <= rhs), true);
    case ND_LOGAND:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs && rhs), true);
    case ND_LOGOR:
        return eval_const_expr(node->lhs, &lhs) && eval_const_expr(node->rhs, &rhs) && ((*val = lhs || rhs), true);
    case ND_NEG:
        return eval_const_expr(node->lhs, &lhs) && ((*val = node->lhs->ty && node->lhs->ty->is_unsigned ? (long long)(-(unsigned long long)lhs) : -lhs), true);
    case ND_NOT:
        return eval_const_expr(node->lhs, &lhs) && ((*val = !lhs), true);
    case ND_BITNOT:
        return eval_const_expr(node->lhs, &lhs) && ((*val = ~lhs), true);
    case ND_CAST: {
        if (!eval_const_expr(node->lhs, val))
            return false;
        if (!node->ty || !is_integer(node->ty))
            return true;
        int sz = node->ty->size;
        if (sz <= 0 || sz >= 8)
            return true;
        int bits = sz * 8;
        unsigned long long mask = (bits == 64) ? ~0ULL : ((1ULL << bits) - 1);
        if (node->ty->is_unsigned) {
            *val &= mask;
        } else {
            *val &= mask;
            if (*val & (1ULL << (bits - 1)))
                *val |= ~mask;
        }
        return true;
    }
    case ND_SIZEOF:
        if (node->lhs && node->lhs->ty) {
            if (node->lhs->ty->kind == TY_VLA)
                return false;
            return (*val = node->lhs->ty->size), true;
        }
        if (node->ty) {
            if (node->ty->kind == TY_VLA)
                return false;
            return (*val = node->ty->size), true;
        }
        return false;
    case ND_ADDR:
        // &*x = x
        if (node->lhs->kind == ND_DEREF)
            return eval_const_expr(node->lhs->lhs, val);
        // offsetof: &((struct S*)0)->member
        return eval_const_addr_expr(node->lhs, val);
    case ND_COMMA:
        return eval_const_expr(node->rhs, val);
    case ND_COND:
        if (!eval_const_expr(node->cond, &lhs))
            return false;
        return eval_const_expr(lhs ? node->then : node->els, val);
    case ND_LVAR:
        if (node->var && node->var->is_constexpr && node->var->has_init) {
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
            while (cur && cur->kind == ND_MEMBER) {
                if (cur->member)
                    total_off += cur->member->offset;
                cur = cur->lhs;
            }
            if (cur && cur->kind == ND_LVAR) {
                root_var = cur->var;
            }
            if (root_var && (root_var->is_constexpr || !root_var->is_local) && root_var->has_init) {
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
            if (is_integer(node->ty) && total_off >= 0) {
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
    default:
        return false;
    }
}

static bool eval_double_const_expr(Node *node, double *val) {
    double lhs, rhs;
    if (!node)
        return false;
    switch (node->kind) {
    case ND_FNUM:
        *val = node->fval;
        return true;
    case ND_NUM:
        *val = (double)node->val;
        return true;
    case ND_ADD:
        if (!eval_double_const_expr(node->lhs, &lhs) || !eval_double_const_expr(node->rhs, &rhs)) return false;
        *val = lhs + rhs;
        if (node->ty && node->ty->kind == TY_FLOAT) *val = (float)*val;
        return true;
    case ND_SUB:
        if (!eval_double_const_expr(node->lhs, &lhs) || !eval_double_const_expr(node->rhs, &rhs)) return false;
        *val = lhs - rhs;
        if (node->ty && node->ty->kind == TY_FLOAT) *val = (float)*val;
        return true;
    case ND_MUL:
        if (!eval_double_const_expr(node->lhs, &lhs) || !eval_double_const_expr(node->rhs, &rhs)) return false;
        *val = lhs * rhs;
        if (node->ty && node->ty->kind == TY_FLOAT) *val = (float)*val;
        return true;
    case ND_DIV:
        if (!eval_double_const_expr(node->lhs, &lhs) || !eval_double_const_expr(node->rhs, &rhs)) return false;
        *val = lhs / rhs;
        if (node->ty && node->ty->kind == TY_FLOAT) *val = (float)*val;
        return true;
    case ND_NEG:
        return eval_double_const_expr(node->lhs, &lhs) && ((*val = -lhs), true);
    case ND_CAST:
        if (!eval_double_const_expr(node->lhs, val)) return false;
        if (node->ty && node->ty->kind == TY_FLOAT) *val = (float)*val;
        if (node->ty && is_integer(node->ty)) *val = (double)(int64_t)*val;
        return true;
    case ND_EQ:
        return eval_double_const_expr(node->lhs, &lhs) && eval_double_const_expr(node->rhs, &rhs) && ((*val = lhs == rhs), true);
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
    default:
        return false;
    }
}

// Evaluate a complex constant expression, extracting real and imag parts.
// Handles patterns like "1.0 + 1.0i", "-2.0 + 2.0i", "1.0 + 14.0 * (1.0fi)", etc.
static bool eval_complex_const_expr(Node *node, double *real_out, double *imag_out) {
    double rl, il, rr, ir;
    if (!node) return false;
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


static Type *declarator(Token **rest, Token *tok, Type *ty, char **name);

static Type *declarator_params(Token **rest, Token *tok, Type *ty) {
    Type param_head = {};
    Type *pcur = &param_head;
    bool is_variadic = false;
    // Save locals so earlier params are visible during VLA dim expressions
    // (e.g. void foo(int a, int b[a++])), then restore afterward.
    LVar *saved_locals = locals;

    if (equalc(tok, "void") && equalc(tok->next, ")")) {
        tok = tok->next->next;
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
            Type *pty = declarator(&tok, tok, copy_type(base), &pname);
            tok = skip_attributes(tok);

            // Preserve VLA dim expression from single-dimension VLA param (e.g. b[a++])
            // so side effects can be emitted at function entry.
            Node *vla_dim_expr = NULL;
            if (pty->kind == TY_VLA) {
                vla_dim_expr = pty->vla_len_expr;
                pty = pointer_to(pty->base);
            } else if (pty->kind == TY_ARRAY) {
                pty = pointer_to(pty->base);
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
                plvar->ty = pt;
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
    *rest = tok;
    return ty;
}

static Type *type_suffix(Token **rest, Token *tok, Type *ty, char *decl_name) {
    int64_t dims[16];
    Node *vla_exprs[16] = {0};
    int ndims = 0;
    while (equalc(tok, "[") && !(equalc(tok->next, "[") && tok->ptr + tok->len == tok->next->ptr)) {
        tok = tok->next;
        int64_t len = 0;
        Node *vla_expr = NULL;
        while (equalc(tok, "const") || equalc(tok, "__const") || equalc(tok, "__const__") ||
               equalc(tok, "volatile") || equalc(tok, "__volatile") || equalc(tok, "__volatile__") ||
               equalc(tok, "restrict") || equalc(tok, "__restrict") || equalc(tok, "__restrict__") ||
               equalc(tok, "_Atomic") || equalc(tok, "__Atomic") ||
               equalc(tok, "static"))
            tok = tok->next;
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
            } else if (equalc(tok, "*")) {
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
    if (!elem || elem->size <= 0 || (!is_integer(elem) && !is_flonum(elem)))
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
    return ty;
}

static Type *declarator(Token **rest, Token *tok, Type *ty, char **name) {
    int decl_align = 0;
    tok = read_type_attrs(tok, &decl_align, NULL);
    if (pending_mode) {
        ty = copy_type(ty);
        int sizes[] = {0, 1, 2, 4, 8};
        ty->size = sizes[pending_mode];
        ty->align = ty->size;
        pending_mode = 0;
    }
    while (equalc(tok, "*")) {
        ty = pointer_to(ty);
        tok = tok->next;
        tok = read_type_attrs(tok, &decl_align, NULL);
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
    if (equalc(tok, "(")) {
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
        tok = after_paren;
        Type *suffixed = type_suffix(&tok, tok, ty, NULL);
        *rest = tok;
        return declarator(&tok, start, suffixed, name);
    }

    // Skip calling convention keywords, attributes, and pointer declarators before the identifier
    for (;;) {
        while (equalc(tok, "__cdecl") || equalc(tok, "__stdcall") || equalc(tok, "__fastcall") ||
               equalc(tok, "__thiscall") || equalc(tok, "__vectorcall"))
            tok = tok->next;
        tok = read_type_attrs(tok, &decl_align, NULL);
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
    tok = read_type_attrs(tok, &decl_align, NULL);
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
    tok = read_type_attrs(tok, NULL, NULL);
    char *tag_name = NULL;
    if (tok->kind == TK_IDENT) {
        tag_name = tok->name;
        tok = tok->next;
    }

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
        // C23 `enum tag : type;` declares the tag with a fixed underlying
        // type — register it so sizeof(enum tag) sees the right size.
        if (tag_name && fixed_underlying) {
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
    int64_t min_val = 0, max_val = 0;
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
            if (!eval_const_expr(node, &v))
                error_tok(tok, "expected constant expression for enum value");
            val = v;
            explicit_val = true;
            expr_ty = node->ty;
        }

        ec->val = val++;
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
        if (ec->ty != ty_int)
            any_outside_int = true;
        prev_ty = ec->ty;
        if (first) {
            min_val = max_val = ec->val;
            first = false;
        } else {
            if (ec->val < min_val) min_val = ec->val;
            if (ec->val > max_val) max_val = ec->val;
        }
        ec->next = enum_consts;
        enum_consts = ec;
        enum_htab_add(ec);

        if (!equalc(tok, "}"))
            tok = skip(tok, ",");
    }

    *rest = tok->next;
    // C23: with a fixed underlying type, the enum uses exactly that type;
    // otherwise choose the narrowest integer type >= int that fits all values.
    Type *ety;
    if (fixed_underlying) {
        ety = fixed_underlying;
    } else if (min_val >= 0) {
        uint64_t umax = (uint64_t)max_val;
        if (umax <= 0xFFFFFFFFULL)
            ety = ty_uint;
        else
            ety = ty_ullong;
    } else {
        if (min_val >= -2147483648LL && max_val <= 2147483647LL)
            ety = ty_int;
        else
            ety = ty_llong;
    }
    Type *ret;
    if (existing_ty) {
        ret = existing_ty; // redefinition: keep the completed type's identity
    } else {
        ret = arena_alloc(sizeof(Type));
        *ret = *ety;
        ret->qual = 0; // qualifiers on a fixed underlying type don't apply
        ret->is_enum = true;
    }
    // C23 6.7.2.2: upon completion the enumerators get the enumerated type,
    // unless all values are representable as int (then they keep type int).
    // With a fixed underlying type they always get the enum type.
    if (fixed_underlying || any_outside_int)
        for (EnumConst *ec = enum_consts; ec && ec != before_consts; ec = ec->next)
            ec->ty = ret;
    // Push tag so subsequent references find the correct type
    if (tag_name) {
        EnumTag *et = arena_alloc(sizeof(EnumTag));
        et->name = tag_name;
        et->ty = ret;
        et->depth = current_block_depth;
        et->members_int = !(fixed_underlying || any_outside_int);
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
    if (!ty || (ty->kind != TY_PTR && ty->kind != TY_VLA))
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

static Type *struct_or_union_specifier(Token **rest, Token *tok, bool is_union) {
    tok = tok->next;
    int struct_attr_align = 0;
    VarAttr struct_attr = {};
    tok = read_type_attrs(tok, &struct_attr_align, &struct_attr);
    char *type_cleanup = pending_cleanup_func;
    pending_cleanup_func = NULL;
    pending_cleanup_tok = NULL;

    Token *tag_tok = NULL;
    if (tok->kind == TK_IDENT) {
        tag_tok = tok;
        tok = tok->next;
    }

    Type *ty = NULL;
    if (tag_tok) {
        TagScope *tag = find_tag(tag_tok);
        if (tag && !equalc(tok, "{")) {
            // Forward-reference use: return existing type
            ty = tag->ty;
        } else if (tag && equalc(tok, "{") && tag->ty->size == 0 && !tag->ty->members) {
            // Completing a forward-declared (incomplete) type: reuse existing Type object
            // so all pointers to it (typedefs, etc.) see the completed definition.
            ty = tag->ty;
        } else {
            // New definition (possibly shadowing an outer-scope tag)
            ty = arena_alloc(sizeof(Type));
            ty->kind = is_union ? TY_UNION : TY_STRUCT;
            ty->size = 0;
            ty->align = 1;
            ty->bitfield_mode = struct_attr.bitfield_mode;
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

    while (!equalc(tok, "}")) {
        VarAttr attr = {};
        pending_constructor = false;
        pending_destructor = false;
        pending_asm_name = NULL;
        pending_alias_target = NULL;
        // C11 _Static_assert / C23 static_assert inside struct/union body
        if (equalc(tok, "_Static_assert") || equalc(tok, "static_assert")) {
            Token *st = tok;
            tok = skip(tok->next, "(");
            Node *cond = conditional(&tok, tok);
            check_type(cond);
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
                    Node *e = conditional(&tok, tok);
                    (void)e;
                }
            }
            tok = skip(tok, ")");
            tok = skip(tok, ";");
            if (!v) error_tok(st, "%s", msg);
            continue;
        }
        Type *base = declspec(&tok, tok, &attr);
        if (attr.is_typedef || attr.is_extern || attr.is_static)
            error_tok(tok, "invalid storage class in member declaration");
        if (!base)
            error_tok(tok, "expected member type");
        if (equalc(tok, ";")) {
            // Anonymous struct/union member: struct { ... }; or union { ... };
            if (base->kind == TY_STRUCT || base->kind == TY_UNION) {
                // bf_unit_size = 0;
                Member *mem = arena_alloc(sizeof(Member));
                mem->name = NULL;
                mem->bit_offset = 0;
                mem->bit_width = 0;
                mem->ty = base;
                if (is_union) {
                    mem->offset = 0;
                    if (max_size < base->size) max_size = base->size;
                } else {
                    int a = base->align;
                    if (struct_pack > 0 && (struct_pack < a || a == 0)) a = struct_pack;
                    offset = align_to(offset, a);
                    mem->offset = offset;
                    offset += base->size;
                    bit_pos = offset * 8;
                    if (max_align < a) max_align = a;
                }
                cur = cur->next = mem;
            }
            tok = tok->next;
            continue;
        }

        for (;;) {
            char *name = NULL;
            Type *mem_ty = declarator(&tok, tok, copy_type(base), &name);
            tok = skip_attributes(tok);

            // Check for bitfield
            int bit_width = 0;
            if (equalc(tok, ":")) {
                tok = tok->next;
                Node *width_node = conditional(&tok, tok);
                long long w;
                if (!eval_const_expr(width_node, &w))
                    error_tok(tok, "bitfield width must be a constant expression");
                bit_width = (int)w;
                if (bit_width < 0 || bit_width > mem_ty->size * 8)
                    error_tok(tok, "bitfield width out of range");
            }

            // Anonymous struct/union member (no name, no bitfield width, aggregate type)
            // e.g. "struct { u8 a, b; };" inside another struct/union
            if (!name && bit_width == 0 && (mem_ty->kind == TY_STRUCT || mem_ty->kind == TY_UNION)) {
                // bf_unit_size = 0;
                Member *mem = arena_alloc(sizeof(Member));
                mem->name = NULL;
                mem->bit_offset = 0;
                mem->bit_width = 0;
                mem->ty = mem_ty;
                if (is_union) {
                    mem->offset = 0;
                    if (max_size < mem_ty->size) max_size = mem_ty->size;
                } else {
                    int a = mem_ty->align;
                    if (struct_pack > 0 && (struct_pack < a || a == 0)) a = struct_pack;
                    offset = align_to(offset, a);
                    mem->offset = offset;
                    offset += mem_ty->size;
                    bit_pos = offset * 8;
                    if (max_align < a) max_align = a;
                }
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
                        // bf_unit_size = unit;
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
                    bool new_run = bit_pos + bit_width > unit_bits ||
                        ((bit_width > 0) == (kind != ms_prev_kind));
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
                    // bf_unit_size = unit;
                    // bf_unit_offset = unit_base;
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

            if (!equalc(tok, ","))
                break;
            tok = tok->next;
        }

        tok = skip(tok, ";");
    }

    tok = skip(tok, "}");
    if (type_cleanup)
        ty->cleanup_func = type_cleanup;
    ty->members = head.next;
    int final_align = max_align;
    if (struct_pack > 0 && struct_pack < max_align)
        final_align = struct_pack;
    if (struct_attr_align > final_align)
        final_align = struct_attr_align;
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
    *rest = tok;
    return ty;
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
            attr->is_typedef = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "extern")) {
            attr->is_extern = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "static")) {
            attr->is_static = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "constexpr")) {
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
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "register")) {
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "auto")) {
            has_auto_seen = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "__thread") || equalc(tok, "_Thread_local") || equalc(tok, "thread_local")) {
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
                ty = copy_type(ty);
                ty->qual |= QUAL_ATOMIC;
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
        if (equalc(tok, "_Decimal32")) {
            is_float = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Decimal64")) {
            is_double = true;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "_Decimal128")) {
            is_double = true;
            long_count = 1;
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
        if (equalc(tok, "_Float128")) {
            // ARM64 Linux: long double is true binary128; x86 long double is
            // 80-bit extended — _Float128 is not supported there
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
            if (!opt_std_version || strcmp(opt_std_version, "202311L") < 0)
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
        if (is_void) {
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
        ty = copy_type(ty);
        ty->qual |= quals;
    }
    // Apply a type-level vector_size attribute to the base type here, not in
    // declarator(), so every declarator of a multi-declarator declaration
    // inherits it: `vector(4,float) f1, f2;` must make f2 a vector too.
    // Trailing per-declarator attributes still apply in declarator().
    if (pending_vector_size) {
        // mode(SI) etc. adjusts the ELEMENT type and must be folded in before
        // the vector is built, or declarator()'s pending_mode handling would
        // shrink the whole vector to the element size.
        if (pending_mode) {
            ty = copy_type(ty);
            int sizes[] = {0, 1, 2, 4, 8};
            ty->size = sizes[pending_mode];
            ty->align = ty->size;
            pending_mode = 0;
        }
        ty = make_vector_type(ty, pending_vector_size);
        pending_vector_size = 0;
    }
    *rest = tok;
    return ty;
}

static Type *type_name(Token **rest, Token *tok) {
    VarAttr attr = {};
    Type *base = declspec(&tok, tok, &attr);
    Type *ty = declarator(&tok, tok, copy_type(base), NULL);
    tok = skip_attributes(tok);
    *rest = tok;
    return ty;
}

static Type *parse_cast_type(Token **rest, Token *tok) {
    tok = skip(tok, "(");
    Type *ty = type_name(&tok, tok);
    *rest = skip(tok, ")");
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

static void append_reloc(LVar *var, int offset, char *label, int addend) {
    Reloc *rel = arena_alloc(sizeof(Reloc));
    rel->offset = offset;
    rel->label = label;
    rel->addend = addend;

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

static bool read_global_label_initializer(Token **rest, Token *tok, char **label, int *addend) {
    if (tok->kind == TK_STR) {
        StrLit *s = new_str_lit(tok->str, tok->len, tok->string_literal_prefix, 1);
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

    // GCC label address: &&label
    if (equalc(tok, "&&") && tok->next && tok->next->kind == TK_IDENT) {
        if (parser_current_fn)
            *label = format(".L.label.%s.%s", parser_current_fn, tok->next->name);
        else
            *label = tok->next->name;
        if (addend) *addend = 0;
        *rest = tok->next->next;
        return true;
    }

    while (is_cast(tok))
        parse_cast_type(&tok, tok);

    if (equalc(tok, "&"))
        tok = tok->next;

    if (tok->kind == TK_IDENT) {
        // Not a symbol reference: enum constants and the true/false/NULL/
        // nullptr keywords fold to integer constants via the expression path.
        if (find_enum_const(tok) || equalc(tok, "true") || equalc(tok, "false") ||
            equalc(tok, "NULL") || equalc(tok, "nullptr"))
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
        {
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
    case ND_STR: {
        StrLit *s = new_str_lit(node->str, strlen(node->str) + 1, 0, 1);
        *label = format(".LC%d", s->id);
        *addend = 0;
        return true;
    }
    case ND_NUM:
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
    case ND_SUB:
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
            mem = mem->next;
            if (mem && equalc(tok, ","))
                tok = tok->next;
            if (ty->kind == TY_UNION)
                break;
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
            tok = skip(tok, "=");
        }
        if (elem_ty && (elem_ty->kind == TY_STRUCT || elem_ty->kind == TY_UNION) && !equalc(tok, "{")) {
            // Heuristic: if the first token is an identifier of struct/union type,
            // or a compound literal, treat it as a single element expression.
            // Otherwise use flat aggregate initialization.
            bool is_struct_expr = false;
            if (tok->kind == TK_IDENT) {
                LVar *var = find_var(tok);
                if (var && var->ty && (var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION))
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
    if (tok->kind == TK_STR) {
        if (tok->string_literal_prefix == 0 || tok->string_literal_prefix == '8')
            return array_of(ty->base, tok->len + 1);
        // For wide strings, count UTF-8 characters (each becomes one wchar)
        return array_of(ty->base, utf8_len(tok->str) + 1);
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
    while (equalc(t, "("))
        t = t->next;
    if (!is_typename(t))
        return NULL;
    Type *ty = type_name(&t, t);
    if (!ty)
        return NULL;
    while (equalc(t, ")"))
        t = t->next;
    if (equalc(t, "{"))
        return t;
    return NULL;
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
    if (mem->ty->kind == TY_ARRAY && !equalc(tok, "{") && !(mem->ty->base->kind == TY_CHAR && tok->kind == TK_STR)) {
        return global_init_flat_array(tok, var, mem->ty, base_offset + mem->offset);
    }
    return global_init_one(tok, var, mem->ty, base_offset + mem->offset);
}

// Initialize one object of type `ty` at `base + offset` in global init data.
// Handles scalars, arrays, structs, compound literals, and flattened init.
static Token *global_init_one(Token *tok, LVar *var, Type *ty, int offset) {
    // String literal for char array
    if (ty->kind == TY_ARRAY && ty->base->kind == TY_CHAR && tok->kind == TK_STR && tok->string_literal_prefix == 0) {
        int len = tok->len + 1; // include embedded NULs and the terminator
        if (ty->size > 0 && len > ty->size) len = ty->size;
        ensure_init_size(var, offset, len);
        memcpy(var->init_data + offset, tok->str, len);
        return tok->next;
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
        return tok->next;
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
                        tok = global_init_one(tok, var, ty->base, offset + sidx * elem_size + sidx2 * ty->base->base->size);
                    else
                        tok = skip_initializer(tok);
                    idx = sidx + 1;
                    continue;
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
                            // Array index designator: [N]
                            tok = tok->next;
                            long long idx = 0;
                            if (!equalc(tok, "]")) {
                                Node *idx_node = conditional(&tok, tok);
                                check_type(idx_node);
                                if (!eval_const_expr(idx_node, &idx))
                                    error_tok(tok, "expected constant expression for array index");
                            }
                            tok = skip(tok, "]");
                            if (cur_ty->kind != TY_ARRAY)
                                error_tok(tok, "array index designator for non-array type");
                            int elem_size = cur_ty->base->size;
                            chain_base += (int)(idx * elem_size);
                            cur_ty = cur_ty->base;
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
        if (!equalc(tok, "}")) // `{}` leaves the (already zeroed) storage as 0
            tok = global_init_one(tok, var, ty, offset);
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
        // &(compound literal) for pointer types
        if (equalc(tok, "&") && find_compound_literal_start(tok->next)) {
            tok = tok->next;
            Token *compound_start = find_compound_literal_start(tok);
            Token *t = tok;
            while (equalc(t, "(")) t = t->next;
            Type *compound_ty = type_name(&t, t);
            while (equalc(t, ")")) t = t->next;
            static int anon_count;
            char *name = format(".Lanon.%d", anon_count++);
            LVar *anon_var = new_var(name, compound_ty, false);
            Token *rest_inner = NULL;
            global_initializer(&rest_inner, compound_start, anon_var);
            tok = rest_inner;
            append_reloc(var, offset, name, 0);
            return tok;
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
        error_tok(tok, "expected constant expression in initializer");
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
        error_tok(tok, "expected constant expression in initializer");
        return tok;
    }
    long long val = 0;
    if (eval_const_expr(node, &val)) {
        write_scalar_bytes(var, offset, ty->size, (int64_t)val);
        return tok;
    }
    error_tok(tok, "expected constant expression in initializer");
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
    if (mem->ty->kind == TY_ARRAY && !equalc(tok, "{") && !(mem->ty->base->kind == TY_CHAR && tok->kind == TK_STR)) {
        return local_init_flat_array(tok, mem_node, mem->ty, cur);
    }
    return local_init_one(tok, mem_node, mem->ty, cur);
}

static Token *local_init_one(Token *tok, Node *lhs, Type *ty, Node **cur) {
    // String literal for char or wide-char array
    if (ty->kind == TY_ARRAY && tok->kind == TK_STR &&
        (ty->base->kind == TY_CHAR || tok->string_literal_prefix != 0)) {
        Node *rhs = assign(&tok, tok);
        Node *assign_node = new_binary(ND_ASSIGN, lhs, rhs, tok);
        check_type(assign_node);
        *cur = (*cur)->next = new_unary(ND_EXPR_STMT, assign_node, tok);
        return tok;
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
                    continue;
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
                while ((equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) ||
                       equalc(tok, "[")) {
                    if (equalc(tok, "[")) {
                        // Array-index step in a designator chain: .arr[idx]...
                        Token *lb = tok;
                        Node *idx = expr(&tok, tok->next);
                        tok = skip(tok, "]");
                        if (chain_ty->kind != TY_ARRAY && chain_ty->kind != TY_PTR) {
                            chain_ok = false;
                            break;
                        }
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
                tok = skip(tok, "=");
                if (!chain_ok || !last_dm) {
                    tok = skip_initializer(tok);
                } else {
                    tok = local_init_one(tok, chain_lhs, chain_ty, cur);
                }
                mem = first_dm ? first_dm->next : NULL;
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

static Node *declaration(Token **rest, Token *tok) {
    // C23 static_assert / C11 _Static_assert
    if (equalc(tok, "static_assert") || equalc(tok, "_Static_assert")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *cond = conditional(&tok, tok);
        check_type(cond);
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
                // Evaluate as expression and skip
                Node *msg_expr = conditional(&tok, tok);
                (void)msg_expr;
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
        Type *ty = declarator(&tok, tok, copy_type(base), &name);
        tok = read_type_attrs(tok, &decl_align, NULL);
        if (decl_align > 0 && ty->kind == TY_FUNC) {
            ty = copy_type(ty);
            ty->align = decl_align;
        }
        char *cleanup = pending_cleanup_func ? pending_cleanup_func : type_level_cleanup;
        pending_cleanup_func = NULL;
        if (!name)
            error_tok(tok, "expected variable name");

        if (ty->kind == TY_FUNC) {
            Type *fty = ty;
            LVar *fn_sym = find_global_name(name);
            if (!fn_sym) {
                fn_sym = new_var(name, pointer_to(fty), false);
                fn_sym->is_extern = true;
                fn_sym->is_function = true;
                fn_sym->is_weak = attr.is_weak;
                fn_sym->is_reproducible = attr.is_reproducible;
                fn_sym->is_unsequenced = attr.is_unsequenced;
            } else {
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
            if (pending_asm_name)
                lvar->asm_name = pending_asm_name;
            lvar->next = locals;
            locals = lvar;
            if (current_block_depth == 1)
                current_fn_scope_locals = locals;
            if (!equalc(tok, ","))
                break;
            tok = tok->next;
            continue;
        }

        if (attr.is_typedef) {
            add_typedef(name, ty);
        } else if (attr.is_static) {
            // Static local variable: create global storage with unique name
            char *asm_label = format(".Lstatic.%d", static_local_counter++);
            if (equalc(tok, "="))
                ty = infer_array_type(ty, tok->next);
            // Global entry for storage
            LVar *gvar = arena_alloc(sizeof(LVar));
            gvar->name = asm_label;
            gvar->ty = ty;
            gvar->is_local = false;
            gvar->is_static = true;
            gvar->next = globals;
            globals = gvar;
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
            if (!gvar) {
                gvar = new_var(name, ty, false);
                gvar->is_extern = true;
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
            var->ty = copy_type(ty);
            var->ty->qual |= QUAL_CONST;
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
            tok = local_init_one(tok, lhs, var->ty, &cur);
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
                // General case: use global_initializer for brace-enclosed init with literals
                global_initializer(&tok, tok, var);
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
                var->ty = copy_type(inferred);
                var->ty->qual |= QUAL_CONST;
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
                var->ty = copy_type(ty);
                var->ty->qual |= QUAL_CONST;
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
            // so the side effect runs once at the declaration (C11 6.7.6.2p5).
            Node *vla_pre = NULL;
            if (parser_current_fn && ty->kind == TY_PTR)
                ty = vla_freeze_dims(ty, &vla_pre, tok);
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
    *rest = skip(tok, ";");
    return head.next ? head.next : new_node(ND_NULL, tok);
}

static Node *compound_stmt_ex(Token **rest, Token *tok, LVar **out_locals) {
    LVar *saved_locals = locals;
    Typedef *saved_typedefs = typedefs;
    TagScope *saved_tags = tags;
    EnumConst *saved_enum_consts = enum_consts;
    TypedefLog *saved_typedef_log = typedef_scope_checkpoint();
    TagLog *saved_tag_log = tag_scope_checkpoint();
    EnumLog *saved_enum_log = enum_scope_checkpoint();
    int saved_block_depth = current_block_depth;

    Node head = {};
    Node *cur = &head;
    tok = skip(tok, "{");
    current_block_depth++;

    while (!equalc(tok, "}")) {
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
        if (equalc(tok, "static_assert") || equalc(tok, "_Static_assert")) {
            cur->next = declaration(&tok, tok);
            while (cur->next)
                cur = cur->next;
            continue;
        }
        // C23 [[attribute]] at statement level
        if (equalc(tok, "[") && equalc(tok->next, "[") && tok->ptr + tok->len == tok->next->ptr) {
            Token *t = tok->next->next;
            int depth = 1;
            while (depth > 0 && t->kind != TK_EOF) {
                if (equalc(t, "[") && equalc(t->next, "[") && t->ptr + t->len == t->next->ptr) depth++;
                else if (equalc(t, "]") && equalc(t->next, "]")) {
                    depth--;
                    t = t->next;
                }
                t = t->next;
            }
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
        cur = cur->next = stmt(&tok, tok);
    }

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    *rest = tok->next;

    if (out_locals)
        *out_locals = locals;
    else
        node->body = append_cleanup_range(node->body, locals, saved_locals, tok);
    current_block_depth = saved_block_depth;
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
    return compound_stmt_ex(rest, tok, NULL);
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

    // Concatenate template string literals
    char buf[4096];
    int len = 0;
    while (tok->kind == TK_STR) {
        if (len + tok->len < (int)sizeof(buf) - 1) {
            memcpy(buf + len, tok->str, tok->len);
            len += tok->len;
        }
        tok = tok->next;
    }
    node->asm_template = str_intern(buf, len);

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
        while (*c == '=' || *c == '+' || *c == '&') c++;
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
    if (equalc(tok, "return")) {
        Node *node = new_node(ND_RETURN, tok);
        node->cleanup_begin = locals;
        node->cleanup_end = current_fn_scope_locals;
        if (equalc(tok->next, ";")) {
            *rest = tok->next->next;
            return node;
        }
        node->lhs = expr(&tok, tok->next);
        *rest = skip(tok, ";");
        return node;
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

            bool has_semi = false;
            for (Token *s = tok; s && !equalc(s, ")"); s = s->next) {
                if (equalc(s, ";")) {
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
                Type *ty = declarator(&tok, tok, copy_type(base), &name);
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
        tok = skip(tok->next, "(");
        EnumConst *saved_enum = enum_consts;
        EnumLog *saved_enum_log = enum_scope_checkpoint();
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        Node *saved_loop = current_loop;
        node->cleanup_end = locals;
        node->continue_cleanup_end = locals;
        current_loop = node;
        node->then = stmt(&tok, tok);
        current_loop = saved_loop;
        enum_scope_restore(saved_enum_log);
        enum_consts = saved_enum;
        *rest = tok;
        return node;
    }

    if (equalc(tok, "do")) {
        Node *node = new_node(ND_DO, tok);
        Node *saved_loop = current_loop;
        node->cleanup_end = locals;
        node->continue_cleanup_end = locals;
        current_loop = node;
        node->then = stmt(&tok, tok->next);
        current_loop = saved_loop;
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
        tok = skip(tok->next, "(");

        if (!equalc(tok, ";")) {
            if (is_typename(tok)) {
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
        node->cleanup_end = for_init_locals;
        node->continue_cleanup_end = for_init_locals;
        current_loop = node;
        node->then = stmt(&tok, tok);
        current_loop = saved_loop;
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

            bool has_semi = false;
            for (Token *s = tok; s && !equalc(s, ")"); s = s->next) {
                if (equalc(s, ";")) {
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
                Type *ty = declarator(&tok, tok, copy_type(base), &name);
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
            Node *saved = current_switch;
            current_switch = node;
            node->then = stmt(&tok, tok);
            current_switch = saved;
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
        Node *saved = current_switch;
        node->cleanup_end = locals;
        current_switch = node;
        node->then = stmt(&tok, tok);
        current_switch = saved;
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
        if (!eval_const_expr(val_node, &v))
            error_tok(tok, "expected constant expression for case");
        node->case_val = v;
        if (equalc(tok, "...")) {
            tok = tok->next;
            Node *end_node = conditional(&tok, tok);
            check_type(end_node);
            long long ev = 0;
            if (!eval_const_expr(end_node, &ev))
                error_tok(tok, "expected constant expression for case range");
            node->case_end = ev;
            node->is_case_range = true;
        }
        tok = skip(tok, ":");
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
        node->lhs = stmt(&tok, tok);
        current_switch->default_case = node;
        *rest = tok;
        return node;
    }

    if (equalc(tok, "break")) {
        Node *node = new_node(ND_BREAK, tok);
        node->cleanup_begin = locals;
        if (tok->next->kind == TK_IDENT) {
            tok = tok->next->next;
            *rest = skip(tok, ";");
        } else {
            *rest = skip(tok->next, ";");
        }
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
            tok = tok->next->next;
            *rest = skip(tok, ";");
        } else {
            *rest = skip(tok->next, ";");
        }
        if (!current_loop)
            error_tok(tok, "stray continue");
        node->cleanup_end = current_loop->continue_cleanup_end;
        return node;
    }

    if (equalc(tok, "goto")) {
        tok = tok->next;
        if (equalc(tok, "*")) {
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
        if (!label)
            add_pending_goto(node->label_name, node);
        *rest = skip(tok->next, ";");
        return node;
    }

    if (tok->kind == TK_IDENT && equalc(tok->next, ":")) {
        Node *node = new_node(ND_LABEL, tok);
        node->label_name = tok->name;
        record_label_scope(node->label_name, locals);
        resolve_pending_gotos(node->label_name, locals);
        tok = tok->next->next;
        tok = skip_attributes(tok);
        // C23: a label may immediately precede a declaration, or appear at the
        // end of a compound statement (before '}').  In those cases the label
        // has an empty (null) statement body and the enclosing block parses the
        // declaration, if any, as the next block item.
        if (equalc(tok, "}") || equalc(tok, "__auto_type") ||
            equalc(tok, "_Static_assert") || equalc(tok, "static_assert") ||
            is_typename(tok)) {
            node->lhs = new_node(ND_NULL, tok);
            *rest = tok;
            return node;
        }
        node->lhs = stmt(&tok, tok);
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
    *rest = skip(tok, ";");
    return node;
}

static bool type_equal(Type *a, Type *b) {
    if (a == b)
        return true;
    if (!a || !b)
        return false;
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
    case TY_ARRAY:
        if (a->size != b->size)
            return false;
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
                if (!type_equal(pa, pb))
                    return false;
                pa = pa->param_next;
                pb = pb->param_next;
            }
            return !pa && !pb;
        }
    case TY_STRUCT:
    case TY_UNION:
        return a == b;
    default:
        return true;
    }
}

static Node *primary(Token **rest, Token *tok) {
    Node *node = NULL;

    if (equalc(tok, "_Generic")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Type *ctrl_ty;
        if (is_typename(tok)) {
            // C2Y / GCC extension: the controlling operand may be a type name.
            // No lvalue conversion is applied, so qualifiers are preserved
            // (e.g. _Generic(const int, int:1, const int:2) selects const int).
            ctrl_ty = type_name(&tok, tok);
        } else {
            Node *ctrl = assign(&tok, tok);
            check_type(ctrl);
            ctrl_ty = ctrl->ty;
            // Apply lvalue/array/function decay
            if (ctrl_ty->kind == TY_ARRAY)
                ctrl_ty = pointer_to(ctrl_ty->base);
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
        while (!equalc(tok, ")")) {
            if (equalc(tok, "default")) {
                tok = skip(tok->next, ":");
                default_expr = assign(&tok, tok);
            } else {
                Type *ty = type_name(&tok, tok);
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
            Node *block = compound_stmt_ex(&tok, tok->next, &block_locals);
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
            if (var)
                node->lhs = new_var_node(var, tok);
            else {
                node->funcname = tok->name;
                LVar *gvar = find_global_name(tok->name);
                if (gvar && gvar->is_function)
                    node->lhs = new_var_node(gvar, tok);
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
            if (!var || !var->is_local) {
                InlinePackFn *ipf = find_inline_pack_fn(fn_tok->name);
                if (ipf)
                    node = inline_pack_call(node, ipf, fn_tok);
            }
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
                LVar *var = find_var(tok);
                if (!var)
                    error_tok(tok, "undeclared variable");
                node = new_var_node(var, tok);
                tok = tok->next;
            }
        }
    } else if (equalc(tok, "&&") && tok->next && tok->next->kind == TK_IDENT) {
        // GCC label address: &&label
        node = new_node(ND_LABEL_VAL, tok);
        node->label_name = tok->next->name;
        node->ty = pointer_to(ty_void);
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
            node = new_fnum(tok->fval, tok);
            if (tok->val == 1)
                node->ty = ty_float;
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
        case 'L': // Wide string
#ifdef _WIN32
            node->ty = pointer_to(ty_ushort);
#else
            node->ty = pointer_to(ty_uint);
#endif
            break;
        case 'u': // char16_t string
        {
            Type *char16_t_type = typedef_find_name("char16_t");
            if (!char16_t_type) {
                // Fallback to unsigned short if not defined
                char16_t_type = ty_ushort;
            }
            node->ty = pointer_to(char16_t_type);
        } break;
        case '8': // u8 string => char8_t (unsigned char) in C23, char before
            if (opt_std_version && strcmp(opt_std_version, "202311L") >= 0)
                node->ty = pointer_to(ty_uchar);
            else
                node->ty = array_of(ty_char, tok->len + 1);
            break;
        case 'U': // char32_t string
        {
            Type *char32_t_type = typedef_find_name("char32_t");
            if (!char32_t_type) {
                // Fallback to unsigned int if not defined
                char32_t_type = ty_uint;
            }
            node->ty = pointer_to(char32_t_type);
        } break;
        default: // Fallback to regular string
            node->ty = pointer_to(ty_char);
            break;
        }
        StrLit *s = new_str_lit(tok->str, tok->len, tok->string_literal_prefix, node->ty->base->size);
        node->str_id = s->id;
        tok = tok->next;
    } else {
        error_tok(tok, "expected an expression");
    }

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
            if (mem->bit_width > 0) {
                int bw = mem->bit_width;
                if (bw < 32 || (bw == 32 && !mem->ty->is_unsigned))
                    mem_node->ty = ty_int;
                else if (bw == 32)
                    mem_node->ty = ty_uint;
                else
                    mem_node->ty = mem->ty;
            } else {
                mem_node->ty = mem->ty;
            }
            node = mem_node;
            tok = tok->next;
            continue;
        }
        if (equalc(tok, "->")) {
            tok = tok->next;
            check_type(node);
            if ((node->ty->kind != TY_PTR && node->ty->kind != TY_ARRAY) ||
                (node->ty->base->kind != TY_STRUCT && node->ty->base->kind != TY_UNION))
                error_tok(tok, "not a pointer to struct or union");
            node = new_unary(ND_DEREF, node, tok);
            check_type(node);
            Member *mem = find_member(node->ty, tok);
            if (!mem)
                error_tok(tok, "no such member");
            Node *mem_node = new_unary(ND_MEMBER, node, tok);
            mem_node->member = mem;
            if (mem->bit_width > 0) {
                int bw = mem->bit_width;
                if (bw < 32 || (bw == 32 && !mem->ty->is_unsigned))
                    mem_node->ty = ty_int;
                else if (bw == 32)
                    mem_node->ty = ty_uint;
                else
                    mem_node->ty = mem->ty;
            } else {
                mem_node->ty = mem->ty;
            }
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
    return new_num(ty->size, tok);
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

static Node *unary(Token **rest, Token *tok) {
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

                long long idx_val;
                if (!rt_expr && ty->kind == TY_ARRAY && tok->kind == TK_NUM) {
                    // Constant subscript on constant-size array
                    idx_val = tok->val;
                    const_offset += (int)(idx_val * ty->base->size);
                    tok = skip(tok->next, "]");
                } else {
                    // Variable subscript or VLA: build runtime expression
                    Node *idx = assign(&tok, tok);
                    tok = skip(tok, "]");
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
        if (!rt_expr)
            return new_num(const_offset, start);
        // Add any trailing const_offset to the runtime expression
        if (const_offset != 0) {
            Node *tail = new_num(const_offset, start);
            check_type(tail);
            rt_expr = new_binary(ND_ADD, rt_expr, tail, start);
            check_type(rt_expr);
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
        node->ty = ty_ulong;
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
    // __builtin_dynamic_object_size(ptr, type) — runtime size via malloc header
    // For known stack/global arrays: compile-time size.
    // For pointers: emit code to read glibc malloc chunk header at runtime.
    if (equalc(tok, "__builtin_dynamic_object_size")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        tok = skip(tok, ",");
        (void)assign(&tok, tok); // type argument (unused)
        *rest = skip(tok, ")");
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
                if (t->kind == TY_ARRAY || t->kind == TY_STRUCT || t->kind == TY_UNION) {
                    Node *node = new_num((int64_t)t->size, start);
                    node->ty = ty_ulong;
                    return node;
                }
            }
        }
        // Heap pointer: emit runtime code to read malloc chunk header.
        // glibc stores chunk size just before the user pointer:
        //   chunk_size = *(size_t*)((char*)ptr - sizeof(size_t))
        //   usable_size = (chunk_size & ~(2*sizeof(size_t)-1)) - 2*sizeof(size_t)
        int ssz = (int)sizeof(size_t);
        // ptr_expr: (size_t)(char*)ptr
        Node *char_ptr = new_unary(ND_CAST, ptr, start);
        char_ptr->ty = pointer_to(ty_char);
        // p_minus_1: (size_t*)((char*)ptr - sizeof(size_t))
        Node *sub = new_binary(ND_SUB, char_ptr, new_num(ssz, start), start);
        check_type(sub);
        Node *size_ptr = new_unary(ND_CAST, sub, start);
        size_ptr->ty = pointer_to(ty_ulong);
        // chunk_size: *(size_t*)(...)
        Node *chunk_size = new_unary(ND_DEREF, size_ptr, start);
        chunk_size->ty = ty_ulong;
        // chunk_size & ~(2*sizeof(size_t)-1)
        uint64_t mask = ~(uint64_t)(2 * ssz - 1);
        Node *masked = new_binary(ND_BITAND, chunk_size, new_num((int64_t)mask, start), start);
        masked->ty = ty_ulong;
        check_type(masked);
        // result: (chunk_size & mask) - 2*sizeof(size_t)
        Node *result = new_binary(ND_SUB, masked, new_num(2 * ssz, start), start);
        result->ty = ty_ulong;
        check_type(result);
        return result;
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
        return new_num(0, tok);
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

        node->ty = pointer_to(ty);
        node = new_unary(ND_DEREF, node, tok);
        return node;
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
    if (equalc(tok, "__atomic_load_n") || equalc(tok, "__atomic_load")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        if (ptr->ty->kind != TY_PTR && ptr->ty->kind != TY_ARRAY)
            error_tok(start, "pointer expected");
        else {
            Type *base = ptr->ty->base;
            if (!base || base->size == 0 || base->size > 8 || (base->size & (base->size - 1)))
                error_tok(start, "integral or integer-sized pointer target type expected");
        }
        tok = skip(tok, ",");
        Node *node = new_node(ND_ATOMIC_LOAD, start);
        node->lhs = ptr;
        node->atomic_ord = parse_memory_order(&tok);
        *rest = skip(tok, ")");
        if (ptr->ty->base)
            node->ty = ptr->ty->base;
        else
            node->ty = ty_int;
        return node;
    }
    if (equalc(tok, "__atomic_store_n") || equalc(tok, "__atomic_store")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *val = assign(&tok, tok);
        check_type(val);
        if (ptr->ty->kind == TY_PTR || ptr->ty->kind == TY_ARRAY) {
            Type *base = ptr->ty->base;
            if (base) {
                if (ty_const(base))
                    warn_tok(start, "assignment of read-only location");
                if (equalc(start, "__atomic_store_n")) {
                    if (!val->ty) {
                    } else if (is_integer(base) && (val->ty->kind == TY_PTR || val->ty->kind == TY_ARRAY))
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
        Node *node = new_node(ND_ATOMIC_STORE, start);
        node->lhs = ptr;
        node->rhs = val;
        tok = skip(tok, ",");
        node->atomic_ord = parse_memory_order(&tok);
        *rest = skip(tok, ")");
        node->ty = ty_void;
        return node;
    }
    if (equalc(tok, "__atomic_exchange_n") || equalc(tok, "__atomic_exchange")) {
        Token *start = tok;
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
        return node;
    }
    if (equalc(tok, "__atomic_compare_exchange_n") || equalc(tok, "__atomic_compare_exchange")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *ptr = assign(&tok, tok);
        check_type(ptr);
        tok = skip(tok, ",");
        Node *expected = assign(&tok, tok);
        check_type(expected);
        tok = skip(tok, ",");
        Node *desired = assign(&tok, tok);
        check_type(desired);
        if ((ptr->ty->kind == TY_PTR || ptr->ty->kind == TY_ARRAY) && ptr->ty->base) {
            Type *base = ptr->ty->base;
            if ((expected->ty->kind == TY_PTR || expected->ty->kind == TY_ARRAY) && expected->ty->base) {
                if (expected->ty->base->size != base->size)
                    error_tok(start, "pointer target type mismatch in argument 2");
            } else {
                error_tok(start, "pointer target type mismatch in argument 2");
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
        if (!base || base->kind == TY_PTR || base->size == 0 || base->size > 8 || (base->size & (base->size - 1))) \
            error_tok(start, "integral or integer-sized pointer target type expected"); \
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
        tok = skip(tok, ",");
        node->atomic_ord = parse_memory_order(&tok);
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
        tok = skip(tok, ",");
        node->atomic_ord = parse_memory_order(&tok);
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
        tok = skip(tok, ",");
        node->atomic_ord = parse_memory_order(&tok);
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
        tok = skip(tok, ",");
        node->atomic_ord = parse_memory_order(&tok);
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
        tok = skip(tok, ",");
        node->atomic_ord = parse_memory_order(&tok);
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
        tok = skip(tok, ",");
        node->atomic_ord = parse_memory_order(&tok);
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
    if (equalc(tok, "__sync_val_compare_and_swap")) {
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
        node->body = new_unary(ND_ADDR, oldval, start);
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_ord2 = MEMORDER_SEQ_CST;
        *rest = skip(tok, ")");
        node->ty = ptr->ty->base ? ptr->ty->base : ty_int;
        return node;
    }
    if (equalc(tok, "__sync_bool_compare_and_swap")) {
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
        node->body = new_unary(ND_ADDR, oldval, start);
        node->atomic_ord = MEMORDER_SEQ_CST;
        node->atomic_ord2 = MEMORDER_SEQ_CST;
        *rest = skip(tok, ")");
        node->ty = ty_bool;
        return node;
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
    if (equalc(tok, "__builtin_choose_expr")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Node *cond = assign(&tok, tok);
        tok = skip(tok, ",");
        Node *expr1 = assign(&tok, tok);
        tok = skip(tok, ",");
        Node *expr2 = assign(&tok, tok);
        *rest = skip(tok, ")");
        // If condition is a compile-time constant, return the appropriate branch
        long long cv = 0;
        if (eval_const_expr(cond, &cv))
            return cv ? expr1 : expr2;
        // Non-constant: generate as runtime ternary
        Node *node = new_node(ND_COND, start);
        node->cond = cond;
        node->then = expr1;
        node->els = expr2;
        return node;
    }
    if (equalc(tok, "__builtin_types_compatible_p")) {
        Token *start = tok;
        tok = skip(tok->next, "(");
        Type *t1 = type_name(&tok, tok);
        tok = skip(tok, ",");
        Type *t2 = type_name(&tok, tok);
        *rest = skip(tok, ")");
        int compat = 0;
        if (t1->kind == t2->kind) {
            switch (t1->kind) {
            case TY_VOID:
            case TY_BOOL: compat = 1; break;
            case TY_CHAR:
            case TY_SHORT:
            case TY_INT:
            case TY_LONG:
            case TY_LLONG:
                compat = t1->size == t2->size && t1->is_unsigned == t2->is_unsigned;
                break;
            case TY_FLOAT:
            case TY_DOUBLE:
            case TY_LDOUBLE:
                compat = t1->size == t2->size;
                break;
            case TY_PTR:
                compat = t1->base && t2->base &&
                    t1->base->kind == t2->base->kind &&
                    t1->base->size == t2->base->size &&
                    t1->base->is_unsigned == t2->base->is_unsigned &&
                    t1->base->qual == t2->base->qual;
                break;
            case TY_ARRAY:
                if (t1->base && t2->base && t1->base->kind == t2->base->kind &&
                    t1->base->size == t2->base->size)
                    compat = t1->array_len == 0 || t2->array_len == 0 ||
                        t1->array_len == t2->array_len;
                break;
            case TY_STRUCT:
            case TY_UNION: compat = t1 == t2; break;
            default: compat = t1->size == t2->size && t1->is_unsigned == t2->is_unsigned;
            }
        }
        return new_num(compat, start);
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
    if (equalc(tok, "+"))
        return unary(rest, tok->next);
    if (equalc(tok, "-"))
        return vector_lower(new_unary(ND_NEG, unary(rest, tok->next), tok));
    if (equalc(tok, "!"))
        return new_unary(ND_NOT, unary(rest, tok->next), tok);
    if (equalc(tok, "~"))
        return vector_lower(new_unary(ND_BITNOT, unary(rest, tok->next), tok));
    if (equalc(tok, "&"))
        return new_unary(ND_ADDR, unary(rest, tok->next), tok);
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
            Type *ty = parse_cast_type(&tok, tok->next);
            *rest = tok;
            if (ty->kind == TY_VLA) {
                // Runtime sizeof for VLA: len * base_size
                Node *len = ty->vla_len_expr ? ty->vla_len_expr : new_num(ty->array_len, tok);
                Node *base_sz = new_num(ty->base->size, tok);
                Node *result = new_binary(ND_MUL, len, base_sz, tok);
                check_type(result);
                return result;
            }
            // VLA-containing struct: vla_len_expr holds the full runtime size expr
            if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->vla_len_expr) {
                check_type(ty->vla_len_expr);
                return ty->vla_len_expr;
            }
            return new_num(ty->size, tok);
        }
    sizeof_expr:;
        Node *node = unary(&tok, tok->next);
        check_type(node);
        *rest = tok;
        if (node->ty->kind == TY_VLA) {
            // Runtime sizeof for VLA: len * base_size
            Node *len = node->ty->vla_len_expr ? node->ty->vla_len_expr : new_num(node->ty->array_len, tok);
            Node *base_sz = new_num(node->ty->base->size, tok);
            Node *result = new_binary(ND_MUL, len, base_sz, tok);
            check_type(result);
            return result;
        }
        if ((node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) && node->ty->vla_len_expr) {
            check_type(node->ty->vla_len_expr);
            return node->ty->vla_len_expr;
        }
        return new_num(node->ty->size, tok);
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
        if (equalc(tok->next, "(") && is_typename(tok->next->next)) {
            Type *ty = parse_cast_type(&tok, tok->next);
            *rest = tok;
            Node *n = new_num(ty->align, start);
            n->ty = ty_ulong; // _Alignof returns size_t (unsigned)
            return n;
        }
        Node *node = unary(&tok, tok->next);
        check_type(node);
        *rest = tok;
        int al = node->ty->align;
        // For function pointers, use the function type's alignment
        if (node->ty->kind == TY_PTR && node->ty->base && node->ty->base->kind == TY_FUNC)
            al = node->ty->base->align;
        Node *n2 = new_num(al, start);
        n2->ty = ty_ulong; // _Alignof returns size_t (unsigned)
        return n2;
    }
    if (is_cast(tok)) {
        Token *start = tok;
        Type *ty = parse_cast_type(&tok, tok);

        // Compound literal: (type){init_list}
        if (equalc(tok, "{")) {
            Token *init_brace_tok = tok;
            (void)init_brace_tok;
            tok = tok->next;

            // For incomplete arrays, count elements first
            if (ty->kind == TY_ARRAY && ty->size == 0 && ty->base) {
                Token *tmp = tok;
                int count = 0;
                int depth = 0;
                while (true) {
                    if (equalc(tmp, "{")) depth++;
                    else if (equalc(tmp, "}")) {
                        if (depth == 0) break;
                        depth--;
                    }
                    if (depth == 0 && (equalc(tmp, ",") || equalc(tmp, "}")))
                        ;
                    else
                        count++;
                    // Advance past comma-separated items
                    if (depth == 0 && equalc(tmp->next, ",")) {
                        tmp = tmp->next->next;
                        continue;
                    }
                    if (depth == 0 && equalc(tmp->next, "}")) {
                        tmp = tmp->next;
                        continue;
                    }
                    tmp = tmp->next;
                }
                // Simple count: count commas + 1
                tmp = tok;
                count = 1;
                depth = 0;
                while (!(depth == 0 && equalc(tmp, "}"))) {
                    if (equalc(tmp, "{")) depth++;
                    else if (equalc(tmp, "}"))
                        depth--;
                    else if (depth == 0 && equalc(tmp, ","))
                        count++;
                    tmp = tmp->next;
                }
                // Handle trailing comma
                Token *before_end = tok;
                for (Token *t = tok; !equalc(t, "}"); t = t->next)
                    before_end = t;
                if (equalc(before_end, ","))
                    count--;
                ty = array_of(ty->base, count);
            }

            // C23: detect storage class specifiers in compound literal types
            // (static, register, thread_local, _Thread_local).
            bool is_storage = false;
            bool is_tls = false;
            for (Token *t = start->next; t && !equalc(t, ")"); t = t->next) {
                if (equalc(t, "static") || equalc(t, "register"))
                    is_storage = true;
                if (equalc(t, "_Thread_local") || equalc(t, "thread_local")) {
                    is_storage = true;
                    is_tls = true;
                }
            }
            static int anon_count;
            char *name = format(".Lanon.%d", anon_count++);
            LVar *var;
            if (is_storage) {
                var = new_var(name, ty, false);
                var->is_static = true;
                var->is_tls = is_tls;
            } else {
                var = new_var(name, ty, true);
            }
            // Storage-class compound literal: initialize at compile time
            if (is_storage) {
                tok = init_brace_tok;
                global_initializer(&tok, tok, var);
            }
            Node *result = new_var_node(var, start);
            if (is_storage)
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
                    Node *idx = new_num(i, start);
                    Node *elem_ptr = new_binary(ND_ADD, new_var_node(var, start), idx, start);
                    Node *deref = new_unary(ND_DEREF, elem_ptr, start);
                    Node *val;
                    // {.member=val,...} as nested struct/union initializer for element
                    if (equalc(tok, "{") && (ty->base->kind == TY_STRUCT || ty->base->kind == TY_UNION)) {
                        // Synthesize (ElemType){...} compound literal for the element
                        Token *fake_start = tok; // tok = '{', the brace
                        tok = tok->next; // skip '{'
                        char *ename = format(".Lanon.%d", anon_count++);
                        LVar *evar = new_var(ename, ty->base, true);
                        Node *ezinit = new_node(ND_ZERO_INIT, start);
                        ezinit->lhs = new_var_node(evar, start);
                        Node *eres = new_binary(ND_COMMA, ezinit, new_var_node(evar, start), start);
                        Member *emem = ty->base->members;
                        while (!equalc(tok, "}")) {
                            if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                                char *mname = tok->next->name;
                                tok = tok->next->next;
                                tok = skip(tok, "=");
                                Member *m = find_member_by_name(ty->base, mname);
                                if (m) {
                                    Node *ma = new_unary(ND_MEMBER, new_var_node(evar, start), fake_start);
                                    ma->member = m;
                                    check_type(ma);
                                    Node *v2 = assign(&tok, tok);
                                    check_type(v2);
                                    Node *a2 = new_binary(ND_ASSIGN, ma, v2, start);
                                    check_type(a2);
                                    eres = new_binary(ND_COMMA, eres, a2, start);
                                    emem = m->next;
                                } else {
                                    assign(&tok, tok);
                                }
                            } else if (emem) {
                                Node *ma = new_unary(ND_MEMBER, new_var_node(evar, start), fake_start);
                                ma->member = emem;
                                check_type(ma);
                                Node *v2 = assign(&tok, tok);
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
                        val = new_binary(ND_COMMA, eres, efinal, start);
                        check_type(val);
                    } else {
                        val = assign(&tok, tok);
                    }
                    check_type(val);
                    Node *asgn = new_binary(ND_ASSIGN, deref, val, start);
                    check_type(asgn);
                    result = new_binary(ND_COMMA, result, asgn, start);
                    check_type(result);
                    i++;
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
                        Token *save = tok;
                        while (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                            char *mname = tok->next->name;
                            Member *m = NULL;
                            for (Member *mm = cur_ty->members; mm; mm = mm->next) {
                                if (mm->name == mname) {
                                    m = mm;
                                    break;
                                }
                            }
                            if (!m) break;
                            found = m;
                            cur_ty = m->ty;
                            tok = tok->next->next;
                        }
                        if (found) {
                            mem = found;
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
                                    Token *val_start = tok;
                                    Node *val = assign(&tok, tok);
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
                        tok = skip(tok, "{");
                        Member *sub = mem->ty->members;
                        while (!equalc(tok, "}")) {
                            Node *var_node = new_var_node(var, start);
                            Node *member_access = new_node(ND_MEMBER, start);
                            member_access->lhs = var_node;
                            member_access->member = mem;
                            member_access->ty = mem->ty;
                            // Designated initializer: .name = value
                            if (equalc(tok, ".") && tok->next && tok->next->kind == TK_IDENT) {
                                char *name = tok->next->name;
                                tok = tok->next->next;
                                tok = skip(tok, "=");
                                Member *m = find_member_by_name(mem->ty, name);
                                if (m) {
                                    Node *inner_access = new_node(ND_MEMBER, start);
                                    inner_access->lhs = member_access;
                                    inner_access->member = m;
                                    inner_access->ty = m->ty;
                                    Node *val = assign(&tok, tok);
                                    check_type(val);
                                    Node *asgn = new_binary(ND_ASSIGN, inner_access, val, start);
                                    asgn->ty = m->ty;
                                    result = new_binary(ND_COMMA, result, asgn, start);
                                    result->ty = ty;
                                } else {
                                    tok = skip_initializer(tok);
                                }
                            } else if (sub) {
                                Node *inner_access = new_node(ND_MEMBER, start);
                                inner_access->lhs = member_access;
                                inner_access->member = sub;
                                inner_access->ty = sub->ty;
                                Node *val = assign(&tok, tok);
                                check_type(val);
                                Node *asgn = new_binary(ND_ASSIGN, inner_access, val, start);
                                asgn->ty = sub->ty;
                                result = new_binary(ND_COMMA, result, asgn, start);
                                result->ty = ty;
                                sub = sub->next;
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
                        tok = skip(tok, "}");
                    } else if (equalc(tok, "{")) {
                        // Extra braces around a scalar initializer (e.g. { { } } for int*)
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
                    global_initializer(&tok, tok, var);
                    if (var->has_init)
                        var->is_constexpr = true;
                    tok = saved_here;
                }
            }
            *rest = tok;
            return result;
        }

        Node *lhs = unary(rest, tok);
        check_type(lhs);
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
// Falls back to ASSIGN(lhs, OP(lhs, rhs)) for bitfields (can't take address of bitfield).
static Node *to_assign(Node *binary) {
    check_type(binary->lhs);
    Token *tok = binary->tok;
    Node *lhs = binary->lhs;
    // Bitfields can't be addressed; the old ASSIGN(lhs, OP(lhs, rhs)) is safe
    // because bitfield member access has no side effects in the lhs path.
    if (lhs->kind == ND_MEMBER && lhs->member && lhs->member->bit_width > 0)
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
        Type *ty = declarator(&tok, tok, copy_type(base), &name);
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
            ty = pointer_to(ty->base);

        LVar *var = new_var(name, ty, true);
        cur = cur->param_next = var;
    }

    *rest = tok;
    return head.param_next;
}

static void global_initializer(Token **rest, Token *tok, LVar *var) {
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

            Token *compound_start = find_compound_literal_start(tok);
            Token *t = tok;
            while (equalc(t, "(")) t = t->next;
            Type *compound_ty = type_name(&t, t);
            while (equalc(t, ")")) t = t->next;
            static int anon_count;
            char *name = format(".Lanon.%d", anon_count++);
            LVar *anon_var = new_var(name, compound_ty, false);
            global_initializer(rest, compound_start, anon_var);
            tok = *rest;
            if (equalc(tok, "}"))
                tok = tok->next;
            while (equalc(tok, ")"))
                tok = tok->next;
            var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
            var->init_size = var->ty->size;
            var->has_init = true;
            append_reloc(var, 0, name, 0);
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
            static int anon_count2;
            char *name = format(".Lanon.%d", anon_count2++);
            LVar *anon_var = new_var(name, compound_ty, false);
            global_initializer(rest, compound_start, anon_var);
            tok = *rest;
            if (equalc(tok, "}"))
                tok = tok->next;
            while (equalc(tok, ")"))
                tok = tok->next;
            var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
            var->init_size = var->ty->size;
            var->has_init = true;
            append_reloc(var, 0, name, 0);
            *rest = tok;
            return;
        }
        // Try parsing as expression and extracting reloc
        Node *node = assign(&tok, tok);
        check_type(node);
        if (extract_reloc(node, &label, &addend)) {
            var->init_data = arena_alloc(var->ty->size ? var->ty->size : 1);
            var->init_size = var->ty->size;
            var->has_init = true;
            if (label)
                append_reloc(var, 0, label, addend);
            else
                var->init_val = addend;
            *rest = tok;
            return;
        }
        error_tok(tok, "unsupported global initializer");
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


        // Try integer constant evaluation
        long long ival = 0;
        if (eval_const_expr(node, &ival)) {
            var->has_init = true;
            var->init_val = (int64_t)ival;
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

        error_tok(tok, "unsupported global initializer");
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
    memset(global_htab, 0, sizeof(global_htab));
    memset(typedef_htab, 0, sizeof(typedef_htab));
    typedef_log = NULL;
    memset(tag_htab, 0, sizeof(tag_htab));
    tag_log = NULL;
    memset(enum_htab, 0, sizeof(enum_htab));
    enum_log = NULL;
    str_lits = NULL;
    TLItem item_head = {};
    TLItem *item_cur = &item_head;

    while (tok->kind != TK_EOF) {

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
        if (tok->kw == ID_ASM || tok->kw == ID___ASM || tok->kw == ID___ASM__) {
            tok = tok->next;
            char *str = parse_toplevel_asm(&tok, tok);
            TLItem *item = arena_alloc(sizeof(TLItem));
            item->kind = TL_ASM;
            item->asm_str = str;
            item_cur = item_cur->next = item;
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
                    Node *e = conditional(&tok, tok);
                    (void)e;
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
            tok = tok->next;
            continue;
        }

        for (;;) {
            int top_decl_align = 0;
            char *name = NULL;
            Type *ty = declarator(&tok, tok, copy_type(base), &name);
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

            if (!name) {
                tok = skip(tok, ";");
                break;
            }

            bool is_func = ty->kind == TY_FUNC || equalc(tok, "(");

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
                    stack_offset = 80;
                    LVar head = {};
                    LVar *cur = &head;
                    int param_index = 0;
                    for (Type *pt = ty->param_types; pt; pt = pt->param_next) {
                        char *pname = pt->name ? pt->name : format("__param%d", param_index++);
                        LVar *lvar;
                        if (pt->vla_len_val) {
                            // Reuse placeholder LVar from declarator_params; update offset
                            // so VLA dim expressions (e.g. a++) reference the correct slot.
                            lvar = (LVar *)pt->vla_len_val;
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
                } else {
                    fty = func_type(ty);
                    bool is_oldstyle = false;
                    locals = NULL;
                    stack_offset = 80;
                    label_scopes = NULL;
                    pending_gotos = NULL;
                    current_switch = NULL;
                    current_loop = NULL;
                    parser_current_fn = name;

                    tok = tok->next;
                    if (!equalc(tok, ")") && !equalc(tok, "...") && !is_typename(tok)) {
                        // K&R function definition: param list has identifiers, not types
                        is_oldstyle = true;
                        // First pass: collect parameter names and declarations
                        typedef struct KRParam KRParam;
                        struct KRParam {
                            KRParam *next;
                            char *name;
                            Type *ty;
                        };
                        KRParam kr_head = {};
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
                        // Parse K&R parameter declarations between ) and {, match by name
                        while (!equalc(tok, "{")) {
                            VarAttr dattr = {};
                            Type *dty = declspec(&tok, tok, &dattr);
                            for (;;) {
                                char *dname = NULL;
                                Type *ddecl = declarator(&tok, tok, copy_type(dty), &dname);
                                if (dname) {
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
                        // Second pass: create LVars with correct types and offsets
                        LVar head = {};
                        LVar *cur = &head;
                        for (KRParam *krp = kr_head.next; krp; krp = krp->next) {
                            if (!krp->ty)
                                krp->ty = ty_int;
                            LVar *var = new_var(krp->name, krp->ty, true);
                            cur = cur->param_next = var;
                        }
                        params = head.param_next;
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
                    if (fty->return_ty && (fty->return_ty->kind == TY_STRUCT || fty->return_ty->kind == TY_UNION || fty->return_ty->kind == TY_COMPLEX)) {
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
                suppress_fn_scope_update = false;

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

                if (fty->return_ty && (fty->return_ty->kind == TY_STRUCT || fty->return_ty->kind == TY_UNION || fty->return_ty->kind == TY_COMPLEX)) {
                    LVar *retbuf = new_var("__retbuf", pointer_to(fty->return_ty), true);
                    retbuf->cleanup_func = NULL;
                }

                // For typedefs like 'typedef int functype(int);', register the type
                if (attr.is_typedef) {
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
                        fn_lvar->is_extern = attr.is_extern || (!attr.is_inline && !attr.is_static);
                        fn_lvar->is_function = true;
                        fn_lvar->is_inline = attr.is_inline;
                        fn_lvar->is_weak = attr.is_weak;
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

                    LVar *fn_locals = NULL;
                    Node *body = compound_stmt_ex(&tok, tok, &fn_locals);
                    // Implicit return 0 for main if no explicit return
                    if (name == kw_main) {
                        Node *last = body->body;
                        if (last) {
                            while (last->next)
                                last = last->next;
                            if (last->kind != ND_RETURN) {
                                Node *ret = new_node(ND_RETURN, tok);
                                ret->lhs = new_num(0, tok);
                                last->next = ret;
                            }
                        } else {
                            // Empty body: insert return 0
                            body->body = new_node(ND_RETURN, tok);
                            body->body->lhs = new_num(0, tok);
                        }
                    }
                    Function *fn = arena_alloc(sizeof(Function));
                    fn->name = name;
                    LVar *fn_sym2 = find_global_name(name);
                    fn->asm_name = pending_asm_name ? pending_asm_name
                                                    : (fn_sym2 ? fn_sym2->asm_name : NULL);
                    fn->alias_target = pending_alias_target;
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
                    fn->stack_size = align_to(stack_offset, 16);
                    fn->is_variadic = is_variadic;
                    fn->dealloc_vla = fn_uses_vla;
                    fn->is_constructor = pending_constructor;
                    fn->is_destructor = pending_destructor;
                    fn->is_inline = attr.is_inline;
                    fn->is_gnu_inline = attr.is_gnu_inline;
                    // is_static is sticky: if any decl was static the fn is static
                    fn->is_static = attr.is_static || (fn_sym2 && fn_sym2->is_static);
                    // is_extern: explicit extern on this def, OR any non-inline extern
                    // declaration seen (has_init flag).
                    fn->is_extern = attr.is_extern || (fn_sym2 && fn_sym2->has_init);
                    fn->is_weak = attr.is_weak || (fn_sym2 && fn_sym2->is_weak);
                    pending_constructor = false;
                    pending_destructor = false;
                    pending_asm_name = NULL;
                    pending_alias_target = NULL;
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
                        item_cur = item_cur->next = item;
                    }
                    current_fn_scope_locals = NULL;
                    current_block_depth = 0;
                    suppress_fn_scope_update = false;
                    break;
                }

                if (equalc(tok, ";")) {
                    pending_asm_name = NULL;
                    pending_alias_target = NULL;
                    tok = tok->next;
                    break;
                }
                if (equalc(tok, ",")) {
                    tok = tok->next;
                    continue;
                }
                error_tok(tok, "expected ';', ',', or '{'");
            } else {
                if (attr.is_typedef) {
                    add_typedef(name, ty);
                } else {
                    if (equalc(tok, "="))
                        ty = infer_array_type(ty, tok->next);
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
                    LVar *var = find_global_name(name);
                    if (var) {
                        if (var->ty->kind == TY_ARRAY && ty->kind == TY_ARRAY && var->ty->size > 0)
                            ty = var->ty;
                        else
                            var->ty = ty;
                    } else {
                        var = new_var(name, ty, false);
                    }
                    // A redeclaration of an already-defined (initialized) global
                    // must not drop its definition or change its linkage: e.g.
                    // `int i = 1; extern int i;` keeps i defined, and
                    // `static auto u = 10U; extern unsigned u;` keeps u static.
                    if (!var->has_init) {
                        var->is_extern = attr.is_extern;
                        var->is_static = attr.is_static;
                        var->is_tls = attr.is_tls;
                    }
                    if (pending_asm_name)
                        var->asm_name = pending_asm_name;
                    if (pending_alias_target)
                        var->alias_target = pending_alias_target;
                    pending_asm_name = NULL;
                    pending_alias_target = NULL;
                    if (equalc(tok, "=")) {
                        tok = tok->next;
                        global_initializer(&tok, tok, var);
                    }
                    if (attr.is_constexpr) {
                        var->is_constexpr = true;
                        // C23 6.2.2: constexpr at file scope gives internal linkage
                        var->is_static = true;
                        // constexpr implies const
                        var->ty = copy_type(ty);
                        var->ty->qual |= QUAL_CONST;
                        if (!var->has_init)
                            error("constexpr variable must be initialized");
                    }
                }

                if (equalc(tok, ";")) {
                    tok = tok->next;
                    break;
                }
                if (equalc(tok, ",")) {
                    tok = tok->next;
                    continue;
                }
                error_tok(tok, "expected ';' or ','");
            }
        }
    }

    Program *prog = arena_alloc(sizeof(Program));
    prog->items = item_head.next;
    prog->globals = globals;
    prog->strs = str_lits;
    return prog;
}
