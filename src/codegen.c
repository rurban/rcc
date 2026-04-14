#include "rcc.h"

static char *current_fn;
static int rcc_label_count = 0;

static char *reg64[] = {"r10", "r11", "rbx", "r12", "r13", "r14", "r15", "rsi"};
static char *reg32[] = {"r10d", "r11d", "ebx", "r12d", "r13d", "r14d", "r15d", "esi"};
static char *reg8[]  = {"r10b", "r11b", "bl", "r12b", "r13b", "r14b", "r15b", "sil"};
static int used_regs = 0;

static int alloc_reg(void) {
    for (int i = 0; i < 8; i++) {
        if ((used_regs & (1 << i)) == 0) {
            used_regs |= (1 << i);
            return i;
        }
    }
    error("Register exhaustion");
    return 0;
}

static void free_reg(int i) {
    used_regs &= ~(1 << i);
}

static int gen(Node *node);

// Generate code to compute the absolute address of an lvalue.
static int gen_addr(Node *node) {
    switch (node->kind) {
    case ND_LVAR: {
        int r = alloc_reg();
        printf("  lea %s, [rbp-%d]\n", reg64[r], node->var->offset);
        return r;
    }
    case ND_DEREF:
        return gen(node->lhs);
    default:
        error_tok(node->tok, "lvalue required as left operand of assignment");
        return -1;
    }
}

// Generate code for a given node.
static int gen(Node *node) {
    if (!node) return -1;

    switch (node->kind) {
    case ND_NUM: {
        int r = alloc_reg();
        printf("  mov %s, %d\n", reg64[r], node->val);
        return r;
    }
    case ND_LVAR: {
        int r = alloc_reg();
        printf("  mov %s, [rbp-%d]\n", reg64[r], node->var->offset);
        return r;
    }
    case ND_ASSIGN: {
        if (node->lhs->kind == ND_LVAR) {
            int r2 = gen(node->rhs);
            printf("  mov [rbp-%d], %s\n", node->lhs->var->offset, reg64[r2]);
            return r2;
        }
        int r1 = gen_addr(node->lhs);
        int r2 = gen(node->rhs);
        printf("  mov [%s], %s\n", reg64[r1], reg64[r2]);
        free_reg(r1);
        return r2;
    }
    case ND_ADDR:
        return gen_addr(node->lhs);
    case ND_STR: {
        int r = alloc_reg();
        printf("  lea %s, [rip + .LC%d]\n", reg64[r], node->str_id);
        return r;
    }
    case ND_DEREF: {
        int r = gen(node->lhs);
        printf("  mov %s, [%s]\n", reg64[r], reg64[r]);
        return r;
    }
    case ND_RETURN: {
        if (node->lhs) {
            int r = gen(node->lhs);
            printf("  mov rax, %s\n", reg64[r]);
            free_reg(r);
        }
        printf("  jmp .L.return.%s\n", current_fn);
        return -1;
    }
    case ND_NULL:
        return -1;
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next) {
            int r = gen(n);
            if (r != -1) free_reg(r);
        }
        return -1;
    case ND_EXPR_STMT: {
        int r = gen(node->lhs);
        if (r != -1) free_reg(r);
        return -1;
    }
    case ND_FUNCALL: {
        int nargs = 0;
        int arg_regs[6];
        for (Node *arg = node->args; arg; arg = arg->next) {
            arg_regs[nargs++] = gen(arg);
        }
        char *argreg64[] = {"rcx", "rdx", "r8", "r9"};
        for (int i = 0; i < nargs; i++) {
            if (i < 4) printf("  mov %s, %s\n", argreg64[i], reg64[arg_regs[i]]);
            free_reg(arg_regs[i]);
        }
        
        printf("  push r10\n");
        printf("  push r11\n");
        printf("  mov rax, 0\n");
        printf("  sub rsp, 32\n");
        printf("  call %s\n", node->funcname);
        printf("  add rsp, 32\n");
        printf("  pop r11\n");
        printf("  pop r10\n");
        
        int r = alloc_reg();
        printf("  mov %s, rax\n", reg64[r]);
        return r;
    }
    case ND_IF: {
        int c = ++rcc_label_count;
        int r = gen(node->cond);
        printf("  cmp %s, 0\n", reg64[r]);
        free_reg(r);
        if (node->els) {
            printf("  je  .L.else.%d\n", c);
            int r1 = gen(node->then);
            if (r1 != -1) free_reg(r1);
            printf("  jmp .L.end.%d\n", c);
            printf(".L.else.%d:\n", c);
            int r2 = gen(node->els);
            if (r2 != -1) free_reg(r2);
            printf(".L.end.%d:\n", c);
        } else {
            printf("  je  .L.end.%d\n", c);
            int r1 = gen(node->then);
            if (r1 != -1) free_reg(r1);
            printf(".L.end.%d:\n", c);
        }
        return -1;
    }
    case ND_FOR: {
        int c = ++rcc_label_count;
        if (node->init) {
            int r = gen(node->init);
            if (r != -1) free_reg(r);
        }
        printf(".L.begin.%d:\n", c);
        if (node->cond) {
            int r = gen(node->cond);
            printf("  cmp %s, 0\n", reg64[r]);
            free_reg(r);
            printf("  je  .L.end.%d\n", c);
        }
        int r_then = gen(node->then);
        if (r_then != -1) free_reg(r_then);
        if (node->inc) {
            int r_inc = gen(node->inc);
            if (r_inc != -1) free_reg(r_inc);
        }
        printf("  jmp .L.begin.%d\n", c);
        printf(".L.end.%d:\n", c);
        return -1;
    }
    default:
        break;
    }

    int r_lhs = gen(node->lhs);
    int r_rhs = gen(node->rhs);

    switch (node->kind) {
    case ND_ADD:
        printf("  add %s, %s\n", reg64[r_lhs], reg64[r_rhs]);
        break;
    case ND_SUB:
        printf("  sub %s, %s\n", reg64[r_lhs], reg64[r_rhs]);
        break;
    case ND_MUL:
        printf("  imul %s, %s\n", reg64[r_lhs], reg64[r_rhs]);
        break;
    case ND_DIV:
        printf("  mov rax, %s\n", reg64[r_lhs]);
        printf("  cqo\n");
        printf("  idiv %s\n", reg64[r_rhs]);
        printf("  mov %s, rax\n", reg64[r_lhs]);
        break;
    case ND_MOD:
        printf("  mov rax, %s\n", reg64[r_lhs]);
        printf("  cqo\n");
        printf("  idiv %s\n", reg64[r_rhs]);
        printf("  mov %s, rdx\n", reg64[r_lhs]); // remainder is in rdx
        break;
    case ND_EQ:
        printf("  cmp %s, %s\n", reg64[r_lhs], reg64[r_rhs]);
        printf("  sete al\n");
        printf("  movzb %s, al\n", reg64[r_lhs]);
        break;
    case ND_NE:
        printf("  cmp %s, %s\n", reg64[r_lhs], reg64[r_rhs]);
        printf("  setne al\n");
        printf("  movzb %s, al\n", reg64[r_lhs]);
        break;
    case ND_LT:
        printf("  cmp %s, %s\n", reg64[r_lhs], reg64[r_rhs]);
        printf("  setl al\n");
        printf("  movzb %s, al\n", reg64[r_lhs]);
        break;
    case ND_LE:
        printf("  cmp %s, %s\n", reg64[r_lhs], reg64[r_rhs]);
        printf("  setle al\n");
        printf("  movzb %s, al\n", reg64[r_lhs]);
        break;
    default:
        error("invalid expression");
    }
    
    free_reg(r_rhs);
    return r_lhs;
}

void codegen(Program *prog) {
    // Assembly header
    printf(".intel_syntax noprefix\n");

    // Emit data section for strings
    if (prog->strs) {
        printf("\n.data\n");
        for (StrLit *s = prog->strs; s; s = s->next) {
            printf(".LC%d:\n", s->id);
            for (int i = 0; s->str[i]; i++) {
                printf("  .byte %d\n", s->str[i]);
            }
            printf("  .byte 0\n"); // null terminator
        }
        printf("\n.text\n");
    }

    for (Function *fn = prog->funcs; fn; fn = fn->next) {
        current_fn = fn->name;
        // Emit function
        printf(".globl %s\n", fn->name);
        printf("%s:\n", fn->name);

        // Prologue
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        
        // Save non-volatile registers
        printf("  push rbx\n");
        printf("  push r12\n");
        printf("  push r13\n");
        printf("  push r14\n");
        printf("  push r15\n");
        printf("  push rsi\n");
        
        int stack_size = fn->stack_size;
        if (stack_size > 0 && stack_size < 48) stack_size = 48; // minimum size check implicitly
        
        if (stack_size % 16 != 0) {
            stack_size += 16 - (stack_size % 16);
        }
        if (stack_size > 0) {
            printf("  sub rsp, %d\n", stack_size);
        }

        // Save registers to local variables (Windows ABI)
        char *argreg64[] = {"rcx", "rdx", "r8", "r9"};
        int i = 0;
        for (LVar *var = fn->params; var; var = var->param_next) {
            if (i < 4) {
                printf("  mov [rbp-%d], %s\n", var->offset, argreg64[i++]);
            }
        }

        // Generate code for each statement in the function
        used_regs = 0; // reset register allocator
        for (Node *n = fn->body; n; n = n->next) {
            int r = gen(n);
            if (r != -1) free_reg(r);
        }

        // Epilogue (return path)
        printf(".L.return.%s:\n", fn->name);
        
        if (stack_size > 0) {
            printf("  add rsp, %d\n", stack_size);
        }
        
        // Restore non-volatile registers
        printf("  pop rsi\n");
        printf("  pop r15\n");
        printf("  pop r14\n");
        printf("  pop r13\n");
        printf("  pop r12\n");
        printf("  pop rbx\n");
        
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
    }
}
