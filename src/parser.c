#include "rcc.h"

// 削除 (lexer.cへ移動済み)

static bool equal(Token *tok, char *op) {
    return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}
static Token *skip(Token *tok, char *op) {
    if (!equal(tok, op))
        error_tok(tok, "expected specific operator");
    return tok->next;
}

// AST nodes allocation
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

static Node *new_num(int val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

static Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = expr;
    return node;
}

// Forward declarations
static Node *stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);
static Node *declaration(Token **rest, Token *tok);

// Local variables handling
static LVar *locals;

static LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (var->name == tok->name)
            return var;
    }
    return NULL;
}

// stmt = "return" expr ";" | "if" "(" expr ")" stmt | "{" stmt* "}" | expr ";"
static Node *stmt(Token **rest, Token *tok) {
    if (equal(tok, "return")) {
        Node *node = new_node(ND_RETURN, tok);
        node->lhs = expr(&tok, tok->next);
        *rest = skip(tok, ";");
        return node;
    }

    if (equal(tok, "if")) {
        Node *node = new_node(ND_IF, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        if (equal(tok, "else")) {
            node->els = stmt(&tok, tok->next);
        }
        *rest = tok;
        return node;
    }

    if (equal(tok, "while")) {
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        *rest = tok;
        return node;
    }

    if (equal(tok, "for")) {
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next, "(");

        if (!equal(tok, ";")) {
            if (equal(tok, "int") || equal(tok, "char")) {
                node->init = declaration(&tok, tok);
            } else {
                node->init = expr(&tok, tok);
                tok = skip(tok, ";");
            }
        } else {
            tok = tok->next;
        }

        if (!equal(tok, ";"))
            node->cond = expr(&tok, tok);
        tok = skip(tok, ";");

        if (!equal(tok, ")"))
            node->inc = expr(&tok, tok);
        tok = skip(tok, ")");

        node->then = stmt(&tok, tok);
        *rest = tok;
        return node;
    }

    if (equal(tok, "{")) {
        Node head = {};
        Node *cur = &head;
        tok = tok->next;
        while (!equal(tok, "}")) {
            if (equal(tok, "int") || equal(tok, "char")) {
                cur = cur->next = declaration(&tok, tok);
            } else {
                cur = cur->next = stmt(&tok, tok);
            }
        }
        Node *node = new_node(ND_BLOCK, tok);
        node->body = head.next;
        *rest = tok->next;
        return node;
    }

    Node *node = new_node(ND_EXPR_STMT, tok);
    node->lhs = expr(&tok, tok);
    *rest = skip(tok, ";");
    return node;
}

// expr = assign
static Node *expr(Token **rest, Token *tok) {
    return assign(rest, tok);
}

// assign = equality ("=" assign)?
static Node *assign(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    if (equal(tok, "="))
        node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next), tok);
    *rest = tok;
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);

    while (true) {
        Token *start = tok;
        if (equal(tok, "==")) {
            node = new_binary(ND_EQ, node, relational(&tok, tok->next), start);
            continue;
        }
        if (equal(tok, "!=")) {
            node = new_binary(ND_NE, node, relational(&tok, tok->next), start);
            continue;
        }
        *rest = tok;
        return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);

    while (true) {
        Token *start = tok;
        if (equal(tok, "<")) {
            node = new_binary(ND_LT, node, add(&tok, tok->next), start);
            continue;
        }
        if (equal(tok, "<=")) {
            node = new_binary(ND_LE, node, add(&tok, tok->next), start);
            continue;
        }
        if (equal(tok, ">")) {
            node = new_binary(ND_LT, add(&tok, tok->next), node, start);
            continue;
        }
        if (equal(tok, ">=")) {
            node = new_binary(ND_LE, add(&tok, tok->next), node, start);
            continue;
        }
        *rest = tok;
        return node;
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);

    while (true) {
        Token *start = tok;
        if (equal(tok, "+")) {
            node = new_binary(ND_ADD, node, mul(&tok, tok->next), start);
            continue;
        }
        if (equal(tok, "-")) {
            node = new_binary(ND_SUB, node, mul(&tok, tok->next), start);
            continue;
        }
        *rest = tok;
        return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tok) {
    Node *node = unary(&tok, tok);

    while (true) {
        Token *start = tok;
        if (equal(tok, "*")) {
            node = new_binary(ND_MUL, node, unary(&tok, tok->next), start);
            continue;
        }
        if (equal(tok, "/")) {
            node = new_binary(ND_DIV, node, unary(&tok, tok->next), start);
            continue;
        }
        if (equal(tok, "%")) {
            node = new_binary(ND_MOD, node, unary(&tok, tok->next), start);
            continue;
        }
        *rest = tok;
        return node;
    }
}

// unary = ("+" | "-" | "*" | "&") unary | primary
static Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+"))
        return unary(rest, tok->next);
    if (equal(tok, "-"))
        return new_binary(ND_SUB, new_num(0, tok), unary(rest, tok->next), tok);
    if (equal(tok, "&"))
        return new_unary(ND_ADDR, unary(rest, tok->next), tok);
    if (equal(tok, "*"))
        return new_unary(ND_DEREF, unary(rest, tok->next), tok);
    return primary(rest, tok);
}

// primary = "(" expr ")" | num | ident
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    if (tok->kind == TK_IDENT) {
        if (equal(tok->next, "(")) {
            Node *node = new_node(ND_FUNCALL, tok);
            node->funcname = tok->name;
            tok = skip(tok->next, "(");
            Node head = {};
            Node *cur = &head;
            while (!equal(tok, ")")) {
                if (cur != &head) tok = skip(tok, ",");
                cur = cur->next = expr(&tok, tok);
            }
            node->args = head.next;
            *rest = skip(tok, ")");
            return node;
        }

        Node *node = new_node(ND_LVAR, tok);
        LVar *lvar = find_lvar(tok);
        if (!lvar) {
            error_tok(tok, "undefined variable\n\033[1;36mnote\033[0m: variable must be declared before use (e.g., 'int x = 0;')");
        }
        node->var = lvar;
        *rest = tok->next;
        return node;
    }

    if (tok->kind == TK_NUM) {
        Node *node = new_num(tok->val, tok);
        *rest = tok->next;
        return node;
    }

    if (tok->kind == TK_STR) {
        Node *node = new_node(ND_STR, tok);
        node->str = tok->str;
        
        extern StrLit *str_lits;
        extern int str_lit_counter;
        
        StrLit *s = arena_alloc(sizeof(StrLit));
        s->str = tok->str;
        s->id = str_lit_counter++;
        s->next = str_lits;
        str_lits = s;
        
        node->str_id = s->id;
        *rest = tok->next;
        return node;
    }

    error_tok(tok, "expected an expression");
    return NULL;
}

static Node *declaration(Token **rest, Token *tok) {
    Type *basety;
    if (equal(tok, "int")) basety = ty_int;
    else if (equal(tok, "char")) basety = ty_char;
    else error_tok(tok, "expected type name");
    
    tok = tok->next; // skip type name
    
    int ptr_level = 0;
    while (equal(tok, "*")) {
        ptr_level++;
        basety = pointer_to(basety);
        tok = tok->next;
    }
    
    if (tok->kind != TK_IDENT) error_tok(tok, "expected variable name");
    
    Token *name_tok = tok;
    LVar *lvar = arena_alloc(sizeof(LVar));
    lvar->next = locals;
    lvar->name = name_tok->name;
    locals = lvar;
    lvar->offset = locals->next ? locals->next->offset + 8 : 56;
    
    // Default variable type setup (externalized logic for type.c mapping)
    // lvar->ty will be handled dynamically or populated right here.
    
    tok = tok->next;
    
    if (equal(tok, ";")) {
        *rest = tok->next;
        return new_node(ND_NULL, name_tok);
    }
    
    if (equal(tok, "=")) {
        tok = tok->next;
        Node *lhs = new_node(ND_LVAR, name_tok);
        lhs->var = lvar;
        Node *rhs = expr(&tok, tok);
        Node *node = new_binary(ND_ASSIGN, lhs, rhs, name_tok);
        node = new_unary(ND_EXPR_STMT, node, name_tok);
        *rest = skip(tok, ";");
        return node;
    }
    
    error_tok(tok, "expected ; or =");
    return NULL;
}

StrLit *str_lits;
int str_lit_counter;

Program *parse(Token *tok) {
    Function head = {};
    Function *cur = &head;

    while (tok->kind != TK_EOF) {
        locals = NULL;

        if (!equal(tok, "int") && !equal(tok, "char")) error_tok(tok, "expected return type");
        tok = tok->next;
        
        if (tok->kind != TK_IDENT) error_tok(tok, "expected function name");
        char *funcname = tok->name;
        tok = tok->next;
        
        tok = skip(tok, "(");
        LVar params_head = {};
        LVar *params_cur = &params_head;
        while (!equal(tok, ")")) {
            if (equal(tok, "...")) {
                tok = tok->next; // skip varargs
                break;
            }
            if (params_cur != &params_head) tok = skip(tok, ",");
            
            if (equal(tok, "...")) {
                tok = tok->next; // skip varargs after comma
                break;
            }
            
            Type *basety;
            if (equal(tok, "int")) basety = ty_int;
            else if (equal(tok, "char")) basety = ty_char;
            else error_tok(tok, "expected parameter type");

            tok = tok->next;
            
            while (equal(tok, "*")) {
                basety = pointer_to(basety);
                tok = tok->next; // skip pointer in arg
            }
            
            if (tok->kind != TK_IDENT) error_tok(tok, "expected parameter name");
            LVar *lvar = arena_alloc(sizeof(LVar));
            lvar->name = tok->name;
            params_cur = params_cur->param_next = lvar;
            
            lvar->next = locals;
            locals = lvar;
            lvar->offset = locals->next ? locals->next->offset + 8 : 56;
            tok = tok->next;
        }
        tok = skip(tok, ")");
        if (equal(tok, ";")) {
            tok = tok->next;
            continue;
        }
        tok = skip(tok, "{");
        
        Node head_node = {};
        Node *cur_node = &head_node;
        while (!equal(tok, "}")) {
            if (equal(tok, "int") || equal(tok, "char")) {
                cur_node = cur_node->next = declaration(&tok, tok);
            } else {
                cur_node = cur_node->next = stmt(&tok, tok);
            }
        }
        tok = skip(tok, "}");
        
        Function *fn = arena_alloc(sizeof(Function));
        fn->name = funcname;
        fn->params = params_head.param_next;
        fn->body = head_node.next;
        fn->stack_size = locals ? locals->offset : 0;
        
        cur = cur->next = fn;
    }
    
    Program *prog = arena_alloc(sizeof(Program));
    prog->funcs = head.next;
    prog->strs = str_lits;
    return prog;
}
