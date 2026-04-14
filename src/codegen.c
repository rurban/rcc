#include "rcc.h"

static char *current_fn;
static int rcc_label_count = 0;

static char *reg64[] = {"r10", "r11", "rbx", "r12", "r13", "r14", "r15", "rsi"};
static char *reg32[] = {"r10d", "r11d", "ebx", "r12d", "r13d", "r14d", "r15d", "esi"};
static char *reg8[]  = {"r10b", "r11b", "bl", "r12b", "r13b", "r14b", "r15b", "sil"};
static int used_regs = 0;

static char *reg(int r, int size) {
    if (size == 1) return reg8[r];
    if (size == 4) return reg32[r];
    return reg64[r];
}

static char *ptr_size(int size) {
    if (size == 1) return "byte ptr";
    if (size == 4) return "dword ptr";
    return "qword ptr";
}

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


static void gen_cond_branch_inv(Node *cond, char *label) {
    if (cond->kind == ND_EQ || cond->kind == ND_NE || cond->kind == ND_LT || cond->kind == ND_LE) {
        int r_lhs = gen(cond->lhs);
        int sz = cond->lhs->ty->size;
        if (cond->rhs->kind == ND_NUM) {
            printf("  cmp %s, %d\n", reg(r_lhs, sz), cond->rhs->val);
        } else if (cond->rhs->kind == ND_LVAR) {
            printf("  cmp %s, %s [rbp-%d]\n", reg(r_lhs, sz), ptr_size(sz), cond->rhs->var->offset);
        } else {
            int r_rhs = gen(cond->rhs);
            printf("  cmp %s, %s\n", reg(r_lhs, sz), reg(r_rhs, sz));
            free_reg(r_rhs);
        }
        free_reg(r_lhs);

        char *jmp = "";
        if (cond->kind == ND_EQ) jmp = "jne";
        else if (cond->kind == ND_NE) jmp = "je";
        else if (cond->kind == ND_LT) jmp = "jge";
        else if (cond->kind == ND_LE) jmp = "jg";
        
        printf("  %s %s\n", jmp, label);
        return;
    }

    int r = gen(cond);
    printf("  cmp %s, 0\n", reg(r, cond->ty->size));
    free_reg(r);
    printf("  je %s\n", label);
}

// Generate code for a given node.
static int gen(Node *node) {
    if (!node) return -1;

    switch (node->kind) {
    case ND_NUM: {
        int r = alloc_reg();
        printf("  mov %s, %d\n", reg(r, node->ty->size), node->val);
        return r;
    }
    case ND_LVAR: {
        int r = alloc_reg();
        printf("  mov %s, %s [rbp-%d]\n", reg(r, node->ty->size), ptr_size(node->ty->size), node->var->offset);
        return r;
    }
    case ND_ASSIGN: {
        if (node->lhs->kind == ND_LVAR) {
            // Optimization: Detect LVAR = LVAR + NUM
            if (node->rhs->kind == ND_ADD && node->rhs->lhs->kind == ND_LVAR && 
                node->rhs->lhs->var == node->lhs->var && node->rhs->rhs->kind == ND_NUM) {
                printf("  add %s [rbp-%d], %d\n", ptr_size(node->lhs->ty->size), node->lhs->var->offset, node->rhs->rhs->val);
                return -1; // This optimization doesn't return a value for use
            }
            if (node->rhs->kind == ND_NUM) {
                printf("  mov %s [rbp-%d], %d\n", ptr_size(node->lhs->ty->size), node->lhs->var->offset, node->rhs->val);
                return -1;
            }
            int r2 = gen(node->rhs);
            printf("  mov [rbp-%d], %s\n", node->lhs->var->offset, reg(r2, node->lhs->ty->size));
            return r2;
        }
        int r1 = gen_addr(node->lhs);
        int r2 = gen(node->rhs);
        printf("  mov [%s], %s\n", reg64[r1], reg(r2, node->lhs->ty->base->size));
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
        int arg_sizes[6];
        for (Node *arg = node->args; arg; arg = arg->next) {
            arg_regs[nargs] = gen(arg);
            arg_sizes[nargs++] = arg->ty->size;
        }
        char *argreg32[] = {"ecx", "edx", "r8d", "r9d"};
        char *argreg64[] = {"rcx", "rdx", "r8", "r9"};
        for (int i = 0; i < nargs; i++) {
            if (i < 4) {
                if (arg_sizes[i] == 4) printf("  mov %s, %s\n", argreg32[i], reg(arg_regs[i], 4));
                else printf("  mov %s, %s\n", argreg64[i], reg(arg_regs[i], 8));
            }
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
        char end_label[32], else_label[32];
        sprintf(end_label, ".L.end.%d", c);
        sprintf(else_label, ".L.else.%d", c);

        if (node->els) {
            gen_cond_branch_inv(node->cond, else_label);
            int r1 = gen(node->then);
            if (r1 != -1) free_reg(r1);
            printf("  jmp %s\n", end_label);
            printf("%s:\n", else_label);
            int r2 = gen(node->els);
            if (r2 != -1) free_reg(r2);
            printf("%s:\n", end_label);
        } else {
            gen_cond_branch_inv(node->cond, end_label);
            int r1 = gen(node->then);
            if (r1 != -1) free_reg(r1);
            printf("%s:\n", end_label);
        }
        return -1;
    }
    case ND_FOR: {
        int c = ++rcc_label_count;
        char begin_label[32], end_label[32];
        sprintf(begin_label, ".L.begin.%d", c);
        sprintf(end_label, ".L.end.%d", c);

        if (node->init) {
            int r = gen(node->init);
            if (r != -1) free_reg(r);
        }
        printf("%s:\n", begin_label);
        if (node->cond) {
            gen_cond_branch_inv(node->cond, end_label);
        }
        int r_then = gen(node->then);
        if (r_then != -1) free_reg(r_then);
        if (node->inc) {
            int r_inc = gen(node->inc);
            if (r_inc != -1) free_reg(r_inc);
        }
        printf("  jmp %s\n", begin_label);
        printf("%s:\n", end_label);
        return -1;
    }
    default:
        break;
    }

    int r_lhs = gen(node->lhs);
    int sz = node->lhs->ty->size;

    // Fused Division/Modulo Optimization
    if (node->kind == ND_DIV || node->kind == ND_MOD) {
        printf("  mov %s, %s\n", sz == 8 ? "rax" : "eax", reg(r_lhs, sz));
        if (sz == 8) printf("  cqo\n");
        else printf("  cdq\n");
        
        if (node->rhs->kind == ND_LVAR) {
            printf("  idiv %s [rbp-%d]\n", ptr_size(sz), node->rhs->var->offset);
        } else if (node->rhs->kind == ND_NUM) {
            // idiv cannot take immediate, must load to a register
            int r_rhs = alloc_reg();
            printf("  mov %s, %d\n", reg(r_rhs, sz), node->rhs->val);
            printf("  idiv %s\n", reg(r_rhs, sz));
            free_reg(r_rhs);
        } else {
            int r_rhs = gen(node->rhs);
            printf("  idiv %s\n", reg(r_rhs, sz));
            free_reg(r_rhs);
        }
        
        printf("  mov %s, %s\n", reg(r_lhs, sz), node->kind == ND_DIV ? (sz == 8 ? "rax" : "eax") : (sz == 8 ? "rdx" : "edx"));
        return r_lhs;
    }

    // Binary operators with potential immediate/memory optimization for RHS
    if (node->kind == ND_ADD || node->kind == ND_SUB || node->kind == ND_MUL || 
        node->kind == ND_EQ || node->kind == ND_NE || node->kind == ND_LT || node->kind == ND_LE) {
        
        char *inst = "";
        if (node->kind == ND_ADD) inst = "add";
        else if (node->kind == ND_SUB) inst = "sub";
        else if (node->kind == ND_MUL) inst = "imul";
        else inst = "cmp";

        if (node->rhs->kind == ND_NUM) {
            printf("  %s %s, %d\n", inst, reg(r_lhs, sz), node->rhs->val);
        } else if (node->rhs->kind == ND_LVAR) {
            printf("  %s %s, %s [rbp-%d]\n", inst, reg(r_lhs, sz), ptr_size(sz), node->rhs->var->offset);
        } else {
            int r_rhs = gen(node->rhs);
            printf("  %s %s, %s\n", inst, reg(r_lhs, sz), reg(r_rhs, sz));
            free_reg(r_rhs);
        }

        if (node->kind == ND_EQ || node->kind == ND_NE || node->kind == ND_LT || node->kind == ND_LE) {
            char *set = "";
            if (node->kind == ND_EQ) set = "sete";
            else if (node->kind == ND_NE) set = "setne";
            else if (node->kind == ND_LT) set = "setl";
            else if (node->kind == ND_LE) set = "setle";
            printf("  %s al\n", set);
            printf("  movzb %s, al\n", reg(r_lhs, sz));
        }
        return r_lhs;
    }

    error("invalid expression %d", node->kind);
    return -1;
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
