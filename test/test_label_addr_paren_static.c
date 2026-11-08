/* `(&&label)` and `__extension__(&&label)` in a static initializer: Ruby's
 * LABEL_PTR(x) is RB_GNUC_EXTENSION(&&LABEL(x)), i.e. a parenthesized
 * label address, in vmtc.inc's `static const void *const
 * insns_address_table[]`. Only the bare and cast forms were recognized, so
 * rcc reported "expected constant expression in initializer" (and the
 * resulting error left later compound literals mis-parsed as file scope).
 */
#include <stdio.h>

static int dispatch(int i) {
    static const void *const tbl[] = {
        &&D,
        (&&A),
        __extension__(&&B),
        ((&&C)),
    };
    goto *tbl[i];
A: return 10;
B: return 20;
C: return 30;
D: return 40;
}

int main(void) {
    if (dispatch(0) != 40 || dispatch(1) != 10 || dispatch(2) != 20 || dispatch(3) != 30) {
        printf("FAIL\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
