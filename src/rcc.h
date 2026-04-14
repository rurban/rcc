#ifndef RCC_H
#define RCC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

//
// Tokenizer / Lexer
//

typedef enum {
    TK_IDENT,   // Identifiers
    TK_PUNCT,   // Punctuators
    TK_NUM,     // Numeric literals
    TK_STR,     // String literals
    TK_EOF,     // End of file
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
    TokenKind kind; // Token kind
    Token *next;    // Next token
    int val;       // If kind is TK_NUM, its value
    char *name;    // If kind is TK_IDENT, its name
    char *str;     // If kind is TK_STR, its contents
    char *loc;     // Token location
    int len;        // Token length
};

// Error reporting
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);

// Allocator / Utils
void *arena_alloc(size_t size);
char *format(char *fmt, ...);
char *str_intern(char *start, int len);

// Lexer entry point
Token *tokenize(char *filename, char *p);

//
// Parser
//

// Type System
typedef enum {
    TY_INT,
    TY_CHAR,
    TY_PTR,
} TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    int size;   // sizeof
    Type *base; // for pointer
};

extern Type *ty_int;
extern Type *ty_char;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);

typedef struct LVar LVar;
struct LVar {
    LVar *next;
    LVar *param_next;
    char *name;
    int offset;
    Type *ty;
};

typedef struct Node Node;
void add_type(Node *node);

typedef enum {
    ND_ADD,       // +
    ND_SUB,       // -
    ND_MUL,       // *
    ND_DIV,       // /
    ND_MOD,       // %
    ND_EQ,        // ==
    ND_NE,        // !=
    ND_LT,        // <
    ND_LE,        // <=
    ND_ASSIGN,    // =
    ND_ADDR,      // &
    ND_DEREF,     // *
    ND_FUNCALL,   // Function call
    ND_LVAR,      // Local variable
    ND_NUM,       // Integer
    ND_RETURN,    // "return"
    ND_IF,        // "if"
    ND_FOR,       // "for" or "while"
    ND_BLOCK,     // { ... }
    ND_EXPR_STMT, // Expression statement
    ND_NULL,      // Empty statement
    ND_STR,       // String literal
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind; // Node kind
    Node *next;    // Next node (for blocks or statements)

    Token *tok;    // Representative token for this node
    Type *ty;      // AST node type

    Node *lhs;     // Left-hand side
    Node *rhs;     // Right-hand side

    // "if" or "for" statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    // Block or arguments
    Node *body;
    Node *args; // Linked list of args

    // Function call
    char *funcname;

    // String literal
    char *str;
    int str_id;

    // Local variable
    LVar *var;

    int val;       // Used if kind == ND_NUM
};

typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    LVar *params;
    Node *body;
    int stack_size;
};

typedef struct StrLit StrLit;
struct StrLit {
    StrLit *next;
    char *str;
    int id;
};

typedef struct Program Program;
struct Program {
    Function *funcs;
    StrLit *strs;
};

// Parser entry point
Program *parse(Token *tok);

//
// CodeGen
//
void codegen(Program *prog);

// Optimizer (CTFE)
void optimize(Program *prog);

#endif // RCC_H
