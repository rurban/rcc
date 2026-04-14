#include "rcc.h"

Type *ty_int = &(Type){TY_INT, 8, NULL};
Type *ty_char = &(Type){TY_CHAR, 1, NULL};

bool is_integer(Type *ty) {
    return ty->kind == TY_INT || ty->kind == TY_CHAR;
}

Type *pointer_to(Type *base) {
    Type *ty = arena_alloc(sizeof(Type));
    ty->kind = TY_PTR;
    ty->size = 8;
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

static void add_type_internal(Node *node) {
    if (!node || node->ty) return;

    add_type_internal(node->lhs);
    add_type_internal(node->rhs);
    add_type_internal(node->cond);
    add_type_internal(node->then);
    add_type_internal(node->els);
    add_type_internal(node->init);
    add_type_internal(node->inc);

    for (Node *n = node->body; n; n = n->next)
        add_type_internal(n);
    for (Node *n = node->args; n; n = n->next)
        add_type_internal(n);

    switch (node->kind) {
    case ND_ADD:
    case ND_SUB: {
        Type *lty = node->lhs->ty;
        Type *rty = node->rhs->ty;
        if (is_integer(lty) && is_integer(rty)) {
            node->ty = ty_int;
            return;
        }
        if (lty->base && is_integer(rty)) {
            node->rhs = new_scale_mul(node->rhs, lty->base->size);
            node->ty = lty;
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
            node->rhs = new_scale_mul(node->rhs, rty->base->size);
            node->ty = rty;
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
        node->ty = ty_int;
        return;
    case ND_ASSIGN:
        if (node->lhs->kind == ND_LVAR) {
            node->lhs->var->ty = node->rhs->ty;
            node->lhs->ty = node->rhs->ty;
        }
        node->ty = node->lhs->ty;
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_NUM:
        node->ty = ty_int;
        return;
    case ND_LVAR:
        if (!node->var->ty) node->var->ty = ty_int;
        node->ty = node->var->ty;
        return;
    case ND_ADDR:
        node->ty = pointer_to(node->lhs->ty);
        return;
    case ND_DEREF:
        if (node->lhs->ty->kind != TY_PTR) {
            error_tok(node->tok, "invalid pointer dereference\n\033[1;36mnote\033[0m: cannot apply '*' to a non-pointer type");
        }
        node->ty = node->lhs->ty->base;
        return;
    case ND_FUNCALL:
        node->ty = ty_int; // assume func returns int
        for (Node *n = node->args; n; n = n->next)
            add_type(n);
        return;
    case ND_STR:
        node->ty = pointer_to(ty_char);
        return;
    case ND_NULL:
        return;
    default:
        return;
    }
}

void add_type(Node *node) {
    add_type_internal(node);
}
