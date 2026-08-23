/* rcc silently accepted a function redeclaration with a CONFLICTING
 * return type (only parameter lists were compared): `char *foo(int);`
 * then `int foo();` compiled clean. C11 6.2.7p2 requires compatible
 * types; gcc errors "conflicting types". Real-world impact: zsh's
 * configure probes `#include <stdlib.h>` + `int ptsname();` against
 * glibc's `char *ptsname(int)` to decide whether /dev/ptmx is usable —
 * the missing error made zsh take its BSD /dev/ptyXX fallback, which
 * cannot open a pty on Linux (every zpty/zle/completion test failed).
 */
char *foo(int);
int foo(); /* must be a conflicting-types error */

int main(void) { return 0; }
