/* GCC Bug #116092 - Should allow --param options to the optimize attribute/pragma
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116092
 */
/* { dg-do compile } */

/* Feature request: gcc rejects --param options in optimize attributes
 * (c-family/c-common.cc only allows -O/-f options).  The warning below
 * documents the current rejection. */
void __attribute__((optimize("--param=max-completely-peel-loop-nest-depth=1")))
bar (void)
{} /* { dg-warning "bad option .--param=max-completely-peel-loop-nest-depth=1. to attribute .optimize." } */