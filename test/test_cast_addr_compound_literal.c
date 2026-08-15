/* A CAST wrapping "&(compound literal)" in a static/global pointer
 * initializer -- e.g. "(void *) &(T){...}" -- reinterprets the literal's
 * address as a different pointer type. rcc's special token-level
 * "&(compound literal)" detection in global_init_one() only recognized
 * a bare leading '&' (or one redundant wrapping paren around it), never
 * a leading type cast, so any cast in front fell through to the general
 * expression parser and failed with "expected constant expression in
 * initializer". Matches njs's njs_symval()/njs_ascii_strval() macro
 * nesting ("(void*) &(njs_value_t){...}").
 */
#include <stdio.h>

typedef struct {
    int type;
    double number;
} val_t;

struct entry {
    int type;
    void *p;
};

struct entry x = {2, (void *)&(val_t){1, 3.5}};

int main(void) {
    val_t *v = x.p;
    if (v->type != 1 || v->number != 3.5) {
        printf("FAIL: cast-prefixed &(compound literal) initializer broken\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
