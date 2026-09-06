/* GCC Bug #102555 - missing -Waddress comparing &p[0] to zero
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102555
 */
/* { dg-do compile } */
/* { dg-options "-pedantic-errors" } */

int f(int *p, int i) {
    return &p[i] == 0;
}

int g(int *p) {
    return &p[0] == 0;
}

#if 0
  z.c: In function ‘f’:
  z.c:3:16: warning: the comparison will always evaluate as ‘false’ for the pointer operand in ‘p + (sizetype)((long unsigned int)i * 4)’ must not be NULL [-Waddress]
     3 |   return &p[i] == 0;
       |                ^~

;; Function f (null)
;; enabled by -tree-original
{
    return p + (sizetype)((long unsigned int)i * 4) == 0B;
}

// ;; Function g (null)
// ;; enabled by -tree-original
{
    return p == 0B;
}
#endif
