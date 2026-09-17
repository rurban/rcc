/* -O1 dead pure-call statement elimination in opt.c's optimize_node():
 * an expression-statement consisting of nothing but a call to a
 * known-pure function (the fn_purity() table: strlen, strcmp, abs,
 * labs, llabs, isdigit, toupper, plus C23 [[unsequenced]]/
 * [[reproducible]] functions) is dead code -- pure means no side
 * effects, and the result is discarded.
 *
 * The one subtlety guarded here: dropping the call drops its argument
 * evaluation too, so the fold must refuse arguments with side effects.
 * `strlen(p++);` as a statement must STILL advance p (real GCC keeps
 * the increment and drops only the call); same for a volatile argument,
 * whose load is observable.
 *
 * Only runs under -O1+; at -O0 the (harmless, result-discarded) calls
 * are emitted literally, which is equally correct. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int main(void) {
    char buf[32] = "hello";

    /* discarded pure calls: dead at -O1, harmless at -O0 */
    strlen(buf);
    strcmp(buf, "hello");
    abs(-3);
    labs(-4L);
    llabs(-5LL);
    isdigit('1');
    toupper('a');

    /* the same calls in value position still work */
    if (strlen(buf) != 5) return 1;
    if (strcmp(buf, "hello") != 0) return 2;
    if (abs(-3) != 3) return 3;
    if (labs(-4L) != 4L) return 4;
    if (llabs(-5LL) != 5LL) return 5;
    if (!isdigit('1')) return 6;
    if (toupper('a') != 'A') return 7;

    /* side-effecting argument: the increment must survive the drop */
    char *p = buf;
    strlen(p++);
    if (p != buf + 1) return 8;

    /* volatile argument: the load must survive the drop */
    volatile int vi = -9;
    abs(vi);
    if (vi != -9) return 9;

    /* a call with side effects is NOT pure and must stay */
    printf("ok\n");
    return 0;
}
