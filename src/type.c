// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from chibicc by Rui Ueyama.
#include "rcc.h"

// clang-format off
Type *ty_void    = &(Type){.kind=TY_VOID,    .size=1,  .align=1};
Type *ty_bool    = &(Type){.kind=TY_BOOL,    .size=1,  .align=1,  .is_unsigned=true};
#if defined(__aarch64__) && !defined(__APPLE__)
Type *ty_char    = &(Type){.kind=TY_CHAR,    .size=1,  .align=1,  .is_unsigned=true};
#else
Type *ty_char    = &(Type){.kind=TY_CHAR,    .size=1,  .align=1};
#endif
Type *ty_uchar   = &(Type){.kind=TY_CHAR,    .size=1,  .align=1,  .is_unsigned=true};
Type *ty_short   = &(Type){.kind=TY_SHORT,   .size=2,  .align=2};
Type *ty_ushort  = &(Type){.kind=TY_SHORT,   .size=2,  .align=2,  .is_unsigned=true};
Type *ty_int     = &(Type){.kind=TY_INT,     .size=4,  .align=4};
Type *ty_uint    = &(Type){.kind=TY_INT,     .size=4,  .align=4,  .is_unsigned=true};
#ifdef _WIN32
Type *ty_long    = &(Type){.kind=TY_LONG,    .size=4,  .align=4};
Type *ty_ulong   = &(Type){.kind=TY_LONG,    .size=4,  .align=4,  .is_unsigned=true};
#else
Type *ty_long    = &(Type){.kind=TY_LONG,    .size=8,  .align=8};
Type *ty_ulong   = &(Type){.kind=TY_LONG,    .size=8,  .align=8,  .is_unsigned=true};
#endif
Type *ty_llong   = &(Type){.kind=TY_LLONG,   .size=8,  .align=8};
Type *ty_ullong  = &(Type){.kind=TY_LLONG,   .size=8,  .align=8,  .is_unsigned=true};
Type *ty_int128  = &(Type){.kind=TY_INT128,  .size=16, .align=16};
Type *ty_uint128 = &(Type){.kind=TY_INT128,  .size=16, .align=16, .is_unsigned=true};
Type *ty_float   = &(Type){.kind=TY_FLOAT,   .size=4,  .align=4};
Type *ty_double  = &(Type){.kind=TY_DOUBLE,  .size=8,  .align=8};
// Apple ARM64: long double is 64-bit (same as double).
// Linux ARM64/x86-64: long double is 128-bit (80-bit x87 padded, or IEEE quad).
#ifdef __APPLE__
Type *ty_ldouble = &(Type){.kind=TY_LDOUBLE, .size=8,  .align=8};
#else
Type *ty_ldouble = &(Type){.kind=TY_LDOUBLE, .size=16, .align=16};
#endif
Type *ty_nullptr_t = &(Type){.kind=TY_NULLPTR_T, .size=8, .align=8};
// clang-format on

bool is_integer(Type *ty) {
    return ty->kind == TY_BOOL || ty->kind == TY_CHAR || ty->kind == TY_SHORT ||
        ty->kind == TY_INT || ty->kind == TY_LONG || ty->kind == TY_LLONG ||
        ty->kind == TY_INT128;
}

bool is_flonum(Type *ty) {
    return ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE || ty->kind == TY_LDOUBLE;
}

bool is_complex(Type *ty) {
    return ty && ty->kind == TY_COMPLEX;
}

Type *complex_type(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_COMPLEX;
    ty->base = base;
    ty->size = base->size * 2;
    ty->align = base->align;
    return ty;
}


bool is_number(Type *ty) {
    return is_integer(ty) || is_flonum(ty) || is_complex(ty);
}

Type *get_integer_type(int size, bool is_unsigned) {
    if (size <= 1)
        return is_unsigned ? ty_uchar : ty_char;
    if (size <= 2)
        return is_unsigned ? ty_ushort : ty_short;
    if (size <= 4)
        return is_unsigned ? ty_uint : ty_int;
    if (size <= 8)
        return is_unsigned ? ty_ullong : ty_llong;
    return is_unsigned ? ty_uint128 : ty_int128;
}

Type *pointer_to(Type *base) {
    Type *ty = arena_alloc(sizeof(Type));
    ty->kind = TY_PTR;
    ty->size = 8;
    ty->align = 8;
    ty->base = base;
    return ty;
}

Type *array_of(Type *base, int64_t len) {
    Type *ty = arena_alloc(sizeof(Type));
    ty->kind = TY_ARRAY;
    ty->size = base->size * len;
    ty->align = base->align;
    ty->base = base;
    return ty;
}

static Node *new_scale_mul(Node *rhs, int size) {
    Node *num = arena_alloc(sizeof(Node));
    num->kind = ND_NUM;
    num->val = size;
    num->ty = ty_int;
    Node *node = arena_alloc(sizeof(Node));
    node->kind = ND_MUL;
    node->lhs = rhs;
    node->rhs = num;
    node->ty = ty_int;
    return node;
}

// Return a Node* computing the runtime size of a VLA type.
// For non-VLA types, returns a compile-time ND_NUM with ty_ulong.
// For VLA types, computes len * base_size recursively.
static Node *vla_size_node(Type *ty) {
    if (ty->kind != TY_VLA) {
        Node *num = arena_alloc(sizeof(Node));
        num->kind = ND_NUM;
        num->val = ty->size;
        num->ty = ty_ulong;
        return num;
    }
    Node *len;
    if (ty->vla_len_expr) {
        len = ty->vla_len_expr;
    } else {
        len = arena_alloc(sizeof(Node));
        len->kind = ND_NUM;
        len->val = ty->array_len;
        len->ty = ty_ulong;
    }
    Node *base = vla_size_node(ty->base);
    Node *node = arena_alloc(sizeof(Node));
    node->kind = ND_MUL;
    node->lhs = len;
    node->rhs = base;
    node->ty = ty_ulong;
    return node;
}

static Type *integer_promotion(Type *ty) {
    if (!is_integer(ty))
        return ty;
    if (ty->size < 4)
        return ty_int;
    return ty;
}

static Type *get_float_type(Type *lhs, Type *rhs) {
    if (lhs->kind == TY_LDOUBLE || rhs->kind == TY_LDOUBLE)
        return ty_ldouble;
    if (lhs->kind == TY_DOUBLE || rhs->kind == TY_DOUBLE)
        return ty_double;
    return ty_float;
}

static int int_rank(Type *ty) {
    switch (ty->kind) {
    case TY_BOOL: return 0;
    case TY_CHAR: return 1;
    case TY_SHORT: return 2;
    case TY_INT: return 3;
    case TY_LONG: return 4;
    case TY_LLONG: return 5;
    case TY_INT128: return 6;
    default: return 3;
    }
}

static Type *usual_arith_type(Type *lhs, Type *rhs) {
    // Mixed scalar+complex: promote to complex
    if (is_complex(lhs) || is_complex(rhs)) {
        if (is_complex(lhs) && is_complex(rhs)) {
            // Both complex: promote to wider base type
            // If same type, return as-is (no promotion needed)
            if (lhs == rhs) return lhs;
            if (lhs->base == rhs->base && lhs->base->size == rhs->base->size &&
                lhs->base->is_unsigned == rhs->base->is_unsigned)
                return lhs;
            Type *common_base = usual_arith_type(lhs->base, rhs->base);
            if (common_base == lhs->base) return lhs;
            if (common_base == rhs->base) return rhs;
            return complex_type(common_base);
        }
        // One complex, one scalar: promote scalar to complex
        Type *cx = is_complex(lhs) ? lhs : rhs;
        Type *sc = is_complex(lhs) ? rhs : lhs;
        if (!is_number(sc)) return cx;
        return cx; // scalar promotes to complex
    }
    if (is_flonum(lhs) || is_flonum(rhs))
        return get_float_type(lhs, rhs);
    lhs = integer_promotion(lhs);
    rhs = integer_promotion(rhs);
    bool is_unsigned = lhs->is_unsigned || rhs->is_unsigned;
    // Pick the type with higher integer rank
    Type *higher = int_rank(lhs) >= int_rank(rhs) ? lhs : rhs;
    if (is_unsigned == higher->is_unsigned)
        return higher;
    return get_integer_type(higher->size, is_unsigned);
}

static void add_type_internal(Node *node);

// C: a null pointer constant is any integer constant expression with value 0
// (casts to integer types are allowed inside an ICE, so (char)0, (bool)0 and
// (enum e)0 all qualify), optionally cast to unqualified void*.
static bool is_null_pointer_constant(Node *n) {
    while (n && n->kind == ND_CAST && n->ty &&
           (is_integer(n->ty) ||
            (n->ty->kind == TY_PTR && n->ty->base &&
             n->ty->base->kind == TY_VOID && n->ty->base->qual == 0)))
        n = n->lhs;
    return n && n->kind == ND_NUM && n->ty && is_integer(n->ty) && n->val == 0;
}

// Some __builtin_* functions are recognized by name in codegen and emit
// values of a specific width/signedness regardless of the (absent) prototype,
// which would otherwise default to plain `int`. Report their true return
// types here so that e.g. ND_RETURN doesn't mis-truncate/sign-extend a
// 64-bit __builtin_bswap64 result down to 32 bits.
static Type *builtin_return_type(const char *name) {
    if (!name) return NULL;
    if (name == bi_bswap16) return ty_ushort;
    if (name == bi_bswap32) return ty_uint;
    if (name == bi_bswap64) return ty_ullong;
    // copysign builtins return double (handled inline in codegen, type must be correct)
    if (name == bi_copysign) return ty_double;
    if (name == bi_copysignf) return ty_float;
    if (name == bi_copysignl) return ty_ldouble;
    if (name == bi_unreachable) return ty_void;
    return NULL;
}

// Return type assumed for a function called WITHOUT a visible prototype.
// C's implicit declaration rule says such a call returns int, but the standard
// libc memory/string allocators return pointers. Assuming int for them produces
// spurious "pointer/integer mismatch" / truncated-pointer diagnostics when a
// header (or header shim) only declares them AFTER their first use — which
// happens e.g. when glibc's fortified <strings.h> defines bcopy() in terms of
// memmove() before <string.h>'s prototype is in scope. Only consulted on the
// implicit path; an explicit declaration always takes precedence.
static Type *implicit_return_type(const char *name) {
    if (!name) return ty_int;
    if (name == bi_s_alloca) return pointer_to(ty_void);
    static const char *ptr_funcs[] = {
        "memcpy",
        "memmove",
        "memset",
        "memchr",
        "strcpy",
        "strncpy",
        "strcat",
        "strncat",
        "strchr",
        "strrchr",
        "strstr",
        "strpbrk",
        "strdup",
        "strndup",
        "malloc",
        "calloc",
        "realloc",
        NULL,
    };
    for (int i = 0; ptr_funcs[i]; i++)
        if (strcmp(name, ptr_funcs[i]) == 0)
            return pointer_to(ty_void);
    return ty_int;
}

// Create composite type from two compatible types (for conditional expressions).
// Recursively merges pointer, array, function, and struct/union types.
static Type *composite_type(Type *t1, Type *t2) {
    if (!t1 || !t2 || t1 == t2)
        return t1;
    if (t1->kind != t2->kind)
        return t1;
    switch (t1->kind) {
    case TY_PTR:
        return pointer_to(composite_type(t1->base, t2->base));
    case TY_ARRAY:
        // Prefer the complete-sized array over incomplete
        if (t1->size == 0 && t2->size > 0) return t2;
        if (t2->size == 0 && t1->size > 0) return t1;
        return t1;
    case TY_FUNC: {
        Type *comp = arena_alloc(sizeof(Type));
        *comp = *t1;
        comp->return_ty = composite_type(t1->return_ty, t2->return_ty);
        // Prefer unspecified params (is_oldstyle) over specified
        if (t1->is_oldstyle && !t2->is_oldstyle) {
            comp->param_types = t1->param_types;
            comp->is_oldstyle = true;
        } else if (!t1->is_oldstyle && t2->is_oldstyle) {
            comp->param_types = t2->param_types;
            comp->is_oldstyle = true;
        }
        return comp;
    }
    case TY_STRUCT:
    case TY_UNION: {
        if (!t1->members || !t2->members) return t1;
        Member *m1 = t1->members, *m2 = t2->members;
        Member head = {}, *cur = &head;
        while (m1 && m2) {
            Member *cm = arena_alloc(sizeof(Member));
            *cm = *m1;
            if (m1->name && m2->name && strcmp(m1->name, m2->name) == 0)
                cm->ty = composite_type(m1->ty, m2->ty);
            cur->next = cm;
            cur = cm;
            m1 = m1->next;
            m2 = m2->next;
        }
        Type *result = arena_alloc(sizeof(Type));
        *result = *t1;
        result->members = head.next;
        // Recompute size/align for the composite struct
        if (result->kind == TY_STRUCT) {
            int64_t off = 0;
            int max_align = 1;
            for (Member *m = result->members; m; m = m->next) {
                if (m->bit_width > 0) {
                    // Bitfield: account for at least the storage unit size
                    int sz = m->ty ? m->ty->size : 4;
                    off += sz;
                    int al = m->ty ? m->ty->align : 4;
                    if (al > max_align) max_align = al;
                    continue;
                }
                if (!m->ty) continue;
                int al = m->ty->align;
                if (al > max_align) max_align = al;
                off = (off + al - 1) / al * al;
                m->offset = (int)off;
                off += m->ty->size;
            }
            result->size = (off + max_align - 1) / max_align * max_align;
            if (result->size == 0) result->size = t1->size > t2->size ? t1->size : t2->size;
            result->align = max_align;
        } else {
            int64_t max_sz = 0;
            int max_al = 1;
            for (Member *m = result->members; m; m = m->next) {
                if (m->ty && m->ty->size > max_sz) max_sz = m->ty->size;
                if (m->ty && m->ty->align > max_al) max_al = m->ty->align;
            }
            result->size = max_sz;
            result->align = max_al;
        }
        return result;
    }
    default:
        return t1;
    }
}

static void insert_arith_cast(Node **operand, Type *to) {
    Node *cast = arena_alloc(sizeof(Node));
    cast->kind = ND_CAST;
    cast->lhs = *operand;
    cast->ty = to;
    cast->tok = (*operand)->tok;
    *operand = cast;
    add_type_internal(cast->lhs);
    add_type_internal(cast);
}

static void add_type_internal(Node *node) {
    if (!node || node->ty) return;

    add_type_internal(node->lhs);
    add_type_internal(node->rhs);
    add_type_internal(node->cond);
    add_type_internal(node->then);
    add_type_internal(node->els);
    add_type_internal(node->init);
    add_type_internal(node->inc);
    // case_next and default_case are internal control-flow chains for
    // codegen dispatch only; the body/next and lhs chains already cover
    // all case nodes.  Skipping them avoids cycles between lhs (forward
    // fall-through) and case_next (backward prepend) in large switches.
    // add_type_internal(node->case_next);
    // add_type_internal(node->default_case);

    for (Node *n = node->body; n; n = n->next)
        add_type_internal(n);
    for (Node *n = node->args; n; n = n->next)
        add_type_internal(n);

    // Atomic nodes: propagate types (no type computation needed here,
    // types are set by the parser in unary())
    switch (node->kind) {
    case ND_ATOMIC_LOAD:
    case ND_ATOMIC_STORE:
    case ND_ATOMIC_EXCHANGE:
    case ND_ATOMIC_CAS:
    case ND_ATOMIC_FENCE:
    case ND_ATOMIC_FETCH_OP:
        return;
    default:
        break;
    }

    // GCC __attribute__((vector_size)) element-wise operators. When an operand
    // is a vector, the result is the vector type and no scalar promotions or
    // casts are inserted — packed codegen (gen_vector) handles the lanes. A
    // vector combined with a scalar broadcasts the scalar to all lanes.
    {
        Type *lvt = node->lhs ? node->lhs->ty : NULL;
        Type *rvt = node->rhs ? node->rhs->ty : NULL;
        bool lv = lvt && lvt->is_vector;
        bool rv = rvt && rvt->is_vector;
        if (lv || rv) {
            switch (node->kind) {
            case ND_ADD:
            case ND_SUB:
            case ND_MUL:
            case ND_DIV:
            case ND_MOD:
            case ND_BITAND:
            case ND_BITOR:
            case ND_BITXOR:
            case ND_SHL: // lane-wise shifts (count vector or broadcast scalar)
            case ND_SHR:
            case ND_EQ: // vector comparisons yield a same-shape mask vector
            case ND_NE:
            case ND_LT:
            case ND_LE:
                node->ty = lv ? lvt : rvt;
                return;
            case ND_NEG:
            case ND_BITNOT:
                node->ty = lvt;
                return;
            default:
                break;
            }
        }
    }

    switch (node->kind) {
    case ND_ADD:
    case ND_SUB: {
        Type *lty = node->lhs->ty;
        Type *rty = node->rhs->ty;
        if (is_number(lty) && is_number(rty)) {
            node->ty = usual_arith_type(lty, rty);
            if (is_flonum(node->ty)) {
                if (is_integer(lty))
                    insert_arith_cast(&node->lhs, node->ty);
                if (is_integer(rty))
                    insert_arith_cast(&node->rhs, node->ty);
            } else if (is_integer(node->ty)) {
                if (node->ty->size > lty->size)
                    insert_arith_cast(&node->lhs, node->ty);
                if (node->ty->size > rty->size)
                    insert_arith_cast(&node->rhs, node->ty);
            }
            return;
        }
        if (lty->base && is_integer(rty)) {
            if (lty->base->kind == TY_VLA) {
                Node *vla_sz = vla_size_node(lty->base);
                Node *mul = arena_alloc(sizeof(Node));
                mul->kind = ND_MUL;
                mul->lhs = node->rhs;
                mul->rhs = vla_sz;
                mul->ty = ty_ullong;
                node->rhs = mul;
            } else {
                node->rhs = new_scale_mul(node->rhs, lty->base->size);
            }
            if (node->rhs->ty->size < 8) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->rhs->ty->is_unsigned ? ty_ullong : ty_llong;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            }
            node->ty = (lty->kind == TY_ARRAY || lty->kind == TY_VLA) ? pointer_to(lty->base) : lty;
            return;
        }
        if (is_integer(lty) && rty->base) {
            // ptr + int
            if (node->kind == ND_SUB) {
                // error: int - ptr is invalid
                node->ty = ty_int; // fallback
                return;
            }
            Node *tmp = node->lhs;
            node->lhs = node->rhs;
            node->rhs = tmp;
            if (rty->base->kind == TY_VLA) {
                Node *vla_sz = vla_size_node(rty->base);
                Node *mul = arena_alloc(sizeof(Node));
                mul->kind = ND_MUL;
                mul->lhs = node->rhs;
                mul->rhs = vla_sz;
                mul->ty = ty_ullong;
                node->rhs = mul;
            } else {
                node->rhs = new_scale_mul(node->rhs, rty->base->size);
            }
            if (node->rhs->ty->size < 8) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->rhs->ty->is_unsigned ? ty_ullong : ty_llong;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            }
            node->ty = (rty->kind == TY_ARRAY || rty->kind == TY_VLA) ? pointer_to(rty->base) : rty;
            return;
        }
        if (lty->base && rty->base) {
            // ptr - ptr
            // For now just output int
            node->ty = ty_int;
            return;
        }
        node->ty = ty_int;
        return;
    }
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_BITAND:
    case ND_BITXOR:
    case ND_BITOR:
        node->ty = usual_arith_type(node->lhs->ty, node->rhs->ty);
        if (is_flonum(node->ty)) {
            if (is_integer(node->lhs->ty))
                insert_arith_cast(&node->lhs, node->ty);
            if (is_integer(node->rhs->ty))
                insert_arith_cast(&node->rhs, node->ty);
        } else if (is_integer(node->ty)) {
            if (node->ty->size > node->lhs->ty->size)
                insert_arith_cast(&node->lhs, node->ty);
            if (node->ty->size > node->rhs->ty->size)
                insert_arith_cast(&node->rhs, node->ty);
        }
        return;
    case ND_NEG:
        node->ty = is_flonum(node->lhs->ty) ? node->lhs->ty : integer_promotion(node->lhs->ty);
        return;
    case ND_NOT:
        node->ty = ty_int;
        return;
    case ND_FNUM:
        return;
    case ND_REAL:
    case ND_IMAG:
        if (node->lhs->ty && node->lhs->ty->kind == TY_COMPLEX)
            node->ty = node->lhs->ty->base;
        else
            node->ty = node->lhs->ty ? node->lhs->ty : ty_int;
        return;
    case ND_SHL:
    case ND_SHR:
        node->ty = integer_promotion(node->lhs->ty);
        return;
    case ND_LOGAND:
    case ND_LOGOR:
        node->ty = ty_int;
        return;
    case ND_ASSIGN:
        if (node->lhs->ty && node->rhs->ty) {
            bool lf = is_flonum(node->lhs->ty);
            bool rf = is_flonum(node->rhs->ty);
            if (node->lhs->ty->kind == TY_BOOL && node->rhs->ty->kind != TY_BOOL) {
                // Storing into a _Bool must normalize to 0/1 (C11 6.3.1.2),
                // not truncate the low byte — 10 assigns as true, not 10.
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if ((lf && !rf) || (!lf && rf) ||
                       (lf && rf && node->lhs->ty->size != node->rhs->ty->size)) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if (!lf && !rf && is_integer(node->lhs->ty) && is_integer(node->rhs->ty) &&
                       node->lhs->ty->size > node->rhs->ty->size) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if (!lf && !rf && is_integer(node->lhs->ty) && is_integer(node->rhs->ty) &&
                       node->rhs->ty->kind == TY_INT128 && node->lhs->ty->kind != TY_INT128) {
                // Truncation from int128 to smaller: insert explicit cast so codegen
                // knows to extract the value from the 128-bit slot.
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if (is_complex(node->lhs->ty) && !is_complex(node->rhs->ty) &&
                       is_number(node->rhs->ty)) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if (!is_complex(node->lhs->ty) && is_complex(node->rhs->ty) &&
                       is_number(node->lhs->ty)) {
                // Assigning a complex value to a non-complex scalar discards
                // the imaginary part (GNU extension): cast rhs down to the
                // real scalar type so codegen loads just the real component.
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            }
        }
        node->ty = node->lhs->ty;
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE: {
        Type *lty = node->lhs->ty;
        Type *rty = node->rhs->ty;
        if (is_number(lty) && is_number(rty)) {
            Type *cmp_ty = usual_arith_type(lty, rty);
            if (is_flonum(cmp_ty)) {
                if (is_integer(lty))
                    insert_arith_cast(&node->lhs, cmp_ty);
                if (is_integer(rty))
                    insert_arith_cast(&node->rhs, cmp_ty);
            } else if (is_integer(cmp_ty) || is_complex(cmp_ty)) {
                if (cmp_ty->size > lty->size)
                    insert_arith_cast(&node->lhs, cmp_ty);
                if (cmp_ty->size > rty->size)
                    insert_arith_cast(&node->rhs, cmp_ty);
            }
        }
        node->ty = ty_int;
        return;
    }
    case ND_SIZEOF:
        node->ty = ty_int;
        return;
    case ND_POST_INC:
    case ND_POST_DEC:
    case ND_PRE_INC:
    case ND_PRE_DEC:
        node->ty = node->lhs->ty;
        return;
    case ND_COND: {
        Type *tty = node->then->ty;
        Type *ety = node->els->ty;
        // If either operand is void, the result is void
        if ((tty && tty->kind == TY_VOID) || (ety && ety->kind == TY_VOID)) {
            node->ty = ty_void;
            return;
        }
        // Null pointer constant: any integer constant expression with value 0
        // (including casts to integer types like (char)0, (bool)0, (enum e)0),
        // optionally cast to unqualified void*.
        bool then_null = is_null_pointer_constant(node->then);
        bool els_null = is_null_pointer_constant(node->els);
        if (then_null && ety && (ety->kind == TY_PTR || ety->kind == TY_ARRAY || ety->kind == TY_VLA)) {
            node->ty = (ety->kind == TY_ARRAY || ety->kind == TY_VLA) ? pointer_to(ety->base) : ety;
            return;
        }
        if (els_null && tty && (tty->kind == TY_PTR || tty->kind == TY_ARRAY || tty->kind == TY_VLA)) {
            node->ty = (tty->kind == TY_ARRAY || tty->kind == TY_VLA) ? pointer_to(tty->base) : tty;
            return;
        }
        // Both pointers: find composite type
        if (tty && ety && (tty->kind == TY_PTR || tty->kind == TY_ARRAY || tty->kind == TY_VLA) &&
            (ety->kind == TY_PTR || ety->kind == TY_ARRAY || ety->kind == TY_VLA)) {
            Type *tbase = (tty->kind == TY_ARRAY || tty->kind == TY_VLA) ? tty->base : tty->base;
            Type *ebase = (ety->kind == TY_ARRAY || ety->kind == TY_VLA) ? ety->base : ety->base;
            // void* combines with any pointer; qualifiers merge from both sides
            if (tbase->kind == TY_VOID || ebase->kind == TY_VOID) {
                // Pick the void* side; if both void*, pick then-side
                Type *vptr = (ebase->kind != TY_VOID) ? tty : ety;
                unsigned char combined = tbase->qual | ebase->qual;
                // C23 (PR98397): carry element qualifiers from pointer-to-array
                // types; before C23 those qualifiers are lost here
                bool c23 = opt_std_version && strcmp(opt_std_version, "202311L") >= 0;
                if (c23 && (tbase->kind == TY_ARRAY || tbase->kind == TY_VLA))
                    combined |= tbase->base->qual;
                if (c23 && (ebase->kind == TY_ARRAY || ebase->kind == TY_VLA))
                    combined |= ebase->base->qual;
                // _Atomic is not a type qualifier for this merge: the
                // composite of `_Atomic void *` and `void *` is `void *`
                combined &= (unsigned char)~QUAL_ATOMIC;
                if (vptr->base->qual != combined) {
                    Type *vbase = arena_alloc(sizeof(Type));
                    *vbase = *vptr->base;
                    vbase->qual = combined;
                    Type *result = arena_alloc(sizeof(Type));
                    *result = *vptr;
                    result->base = vbase;
                    node->ty = result;
                } else {
                    node->ty = vptr;
                }
            } else {
                // Same base kind: prefer the complete side (for incomplete arrays),
                // then combine qualifiers
                Type *chosen_ptr = (tty->kind == TY_ARRAY || tty->kind == TY_VLA) ? pointer_to(tty->base) : tty;
                Type *other_ptr = (ety->kind == TY_ARRAY || ety->kind == TY_VLA) ? pointer_to(ety->base) : ety;
                // If then-base is an incomplete array and els-base is complete, use els
                if (tbase->kind == TY_ARRAY && tbase->size == 0 &&
                    ebase->kind == TY_ARRAY && ebase->size > 0)
                    chosen_ptr = other_ptr;
                // For struct/union types with different scope definitions but same tag,
                // create a composite type with merged member types
                if ((tbase->kind == TY_STRUCT || tbase->kind == TY_UNION) && tbase != ebase) {
                    Type *comp_base = composite_type(tbase, ebase);
                    unsigned char combined_qual = tbase->qual | ebase->qual;
                    combined_qual &= (unsigned char)~QUAL_ATOMIC;
                    if (comp_base->qual != combined_qual) {
                        Type *rb = arena_alloc(sizeof(Type));
                        *rb = *comp_base;
                        rb->qual = combined_qual;
                        Type *rp = arena_alloc(sizeof(Type));
                        *rp = *chosen_ptr;
                        rp->base = rb;
                        node->ty = rp;
                    } else {
                        Type *rp = arena_alloc(sizeof(Type));
                        *rp = *chosen_ptr;
                        rp->base = comp_base;
                        node->ty = rp;
                    }
                    return;
                }
                unsigned char combined = chosen_ptr->base->qual | (chosen_ptr == other_ptr ? tbase->qual : ebase->qual);
                if (chosen_ptr->base->qual != combined) {
                    Type *rbase = arena_alloc(sizeof(Type));
                    *rbase = *chosen_ptr->base;
                    rbase->qual = combined;
                    Type *rptr = arena_alloc(sizeof(Type));
                    *rptr = *chosen_ptr;
                    rptr->base = rbase;
                    node->ty = rptr;
                } else {
                    node->ty = chosen_ptr;
                }
            }
            // C23: propagate [[reproducible]]/[[unsequenced]] to composite pointer type
            if (node->ty && node->ty->kind == TY_PTR && node->ty->base && node->ty->base->kind == TY_FUNC) {
                Type *fcopy = NULL;
                if (tty && tty->kind == TY_PTR && tty->base && tty->base->kind == TY_FUNC &&
                    (tty->base->is_reproducible || tty->base->is_unsequenced)) {
                    fcopy = arena_alloc(sizeof(Type));
                    *fcopy = *node->ty->base;
                    fcopy->is_reproducible |= tty->base->is_reproducible;
                    fcopy->is_unsequenced |= tty->base->is_unsequenced;
                }
                if (ety && ety->kind == TY_PTR && ety->base && ety->base->kind == TY_FUNC &&
                    (ety->base->is_reproducible || ety->base->is_unsequenced)) {
                    if (!fcopy) {
                        fcopy = arena_alloc(sizeof(Type));
                        *fcopy = *node->ty->base;
                    }
                    fcopy->is_reproducible |= ety->base->is_reproducible;
                    fcopy->is_unsequenced |= ety->base->is_unsequenced;
                }
                if (fcopy) {
                    Type *rptr = arena_alloc(sizeof(Type));
                    *rptr = *node->ty;
                    rptr->base = fcopy;
                    node->ty = rptr;
                }
            }
            return;
        }
        // Both arithmetic: usual arithmetic conversions
        if (tty && ety && is_number(tty) && is_number(ety)) {
            node->ty = usual_arith_type(tty, ety);
            return;
        }
        // Pointer/integer mismatch: warn and use the pointer type
        if (tty && ety && ((tty->kind == TY_PTR && is_integer(ety)) || (ety->kind == TY_PTR && is_integer(tty)))) {
            if (node->tok)
                warn_tok(node->tok, "pointer/integer mismatch in conditional expression");
        }
        node->ty = tty ? tty : ety;
        return;
    }
    case ND_COMMA:
        node->ty = node->rhs->ty;
        // Comma expressions are never lvalues; apply array/function decay
        if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_VLA)
            node->ty = pointer_to(node->ty->base);
        else if (node->ty->kind == TY_FUNC)
            node->ty = pointer_to(node->ty);
        return;
    case ND_NUM:
        node->ty = ty_int;
        return;
    case ND_LVAR:
        if (!node->var->ty) node->var->ty = ty_int;
        node->ty = node->var->ty;
        return;
    case ND_ADDR: {
        Node *operand = node->lhs;
        // A function designator is already represented as pointer-to-function
        // (rcc stores functions as pointer_to(TY_FUNC)). Per C, &func has the
        // same type as the decayed function designator — pointer-to-function,
        // not pointer-to-pointer-to-function. Without this, &func fails to
        // match a `T (*)(...)` type in _Generic and mis-selects the default.
        if (operand->kind == ND_LVAR && operand->var && operand->var->is_function &&
            operand->ty && operand->ty->kind == TY_PTR && operand->ty->base &&
            operand->ty->base->kind == TY_FUNC)
            node->ty = operand->ty;
        else
            node->ty = pointer_to(operand->ty);
        return;
    }
    case ND_DEREF:
        if (node->lhs->ty->kind != TY_PTR && node->lhs->ty->kind != TY_ARRAY && node->lhs->ty->kind != TY_VLA) {
            error_tok(node->tok, "invalid pointer dereference\n\033[1;36mnote\033[0m: cannot apply '*' to a non-pointer type");
        }
        node->ty = node->lhs->ty->base;
        return;
    case ND_CAST:
        if (!node->ty)
            node->ty = node->lhs->ty;
        return;
    case ND_BITNOT:
        node->ty = integer_promotion(node->lhs->ty);
        return;
    case ND_FUNCALL:
        if (node->funcname && builtin_return_type(node->funcname)) {
            node->ty = builtin_return_type(node->funcname);
        } else if (node->lhs && node->lhs->ty) {
            if (node->lhs->ty->kind == TY_PTR &&
                node->lhs->ty->base && node->lhs->ty->base->kind == TY_FUNC) {
                node->ty = node->lhs->ty->base->return_ty;
            } else if (node->lhs->ty->kind == TY_FUNC) {
                node->ty = node->lhs->ty->return_ty;
            } else if (node->funcname) {
                LVar *gvar = find_global_name(node->funcname);
                node->ty = (gvar && gvar->ty && gvar->ty->kind == TY_FUNC && gvar->ty->return_ty)
                    ? gvar->ty->return_ty
                    : implicit_return_type(node->funcname);
            } else {
                node->ty = ty_int;
            }
        } else if (node->funcname) {
            LVar *gvar = find_global_name(node->funcname);
            node->ty = (gvar && gvar->ty && gvar->ty->kind == TY_FUNC && gvar->ty->return_ty)
                ? gvar->ty->return_ty
                : implicit_return_type(node->funcname);
        } else {
            node->ty = ty_int;
        }
        for (Node *n = node->args; n; n = n->next)
            check_type(n);
        return;
    case ND_STR:
        node->ty = pointer_to(ty_char);
        return;
    case ND_MEMBER:
        if (node->member->bit_width > 0) {
            int bw = node->member->bit_width;
            bool bf_unsigned = node->member->ty->is_unsigned;
            // 64-bit declared type (long long / unsigned long long): keep as-is
            // so that sizeof(s->field + 0) == 8 and %016llx format is selected
            if (node->member->ty->size >= 8)
                node->ty = node->member->ty;
            else if (bw < 32 || (bw == 32 && !bf_unsigned))
                node->ty = ty_int;
            else if (bw == 32)
                node->ty = ty_uint;
            else
                node->ty = node->member->ty;
        } else {
            node->ty = node->member->ty;
        }
        return;
    case ND_STMT_EXPR: {
        if (node->stmt_expr_result) {
            add_type_internal(node->stmt_expr_result);
            node->ty = node->stmt_expr_result->ty;
        } else {
            node->ty = ty_int;
        }
        return;
    }
    case ND_DO:
    case ND_SWITCH:
    case ND_CASE:
    case ND_BREAK:
    case ND_CONTINUE:
    case ND_GOTO:
    case ND_GOTO_IND:
    case ND_LABEL:
    case ND_NULL:
    case ND_ZERO_INIT:
        return;
    case ND_LABEL_VAL:
        // type is already set to void* in parser
        return;
    default:
        return;
    }
}

void check_type(Node *node) {
    add_type_internal(node);
}

Type *vla_of(Type *base, Node *len, int64_t arr_len) {
    Type *ty = arena_alloc(sizeof(Type));
    ty->kind = TY_VLA;
    ty->size = 16; // 8 for array base ptr + 8 for saved RSP
    ty->align = 8;
    ty->base = base;
    if (len) {
        check_type(len);
        ty->vla_len_expr = len;
    } else {
        ty->array_len = arr_len;
    }
    return ty;
}
