/* -O1 jump threading in codegen.c: a jmp/jcc that lands on a label whose
 * first instruction is an unconditional jmp can target that final
 * destination directly. The trampoline itself remains a valid landing
 * pad (computed goto/debug symbols may still name it); only its incoming
 * branches are retargeted.
 *
 * k() has the minimal form:
 *     goto a;
 *   a: goto end;
 *   end: return x;
 * `goto a` must bypass a's unconditional jump at -O1. m() covers both
 * the taken and untaken conditional path across the same style of local
 * labels. At -O0 the original trampolines remain, with identical results. */
int k(int x) {
    goto a;
a:
    goto end;
end:
    return x;
}

int m(int x) {
    int r = 0;
    if (x > 0)
        goto taken;
    goto done;
taken:
    r = 1;
done:
    if (x == 999)
        goto special;
    return r;
special:
    return 42;
}

int main(void) {
    if (k(-7) != -7) return 1;
    if (k(5) != 5) return 2;
    if (m(-1) != 0) return 3;
    if (m(0) != 0) return 4;
    if (m(1) != 1) return 5;
    if (m(999) != 42) return 6;
    return 0;
}
