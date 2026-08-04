/* GCC Bug #68637 - Array of function pointers with attribute leads to wrong code
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68637
 */


extern void (*bar[]) (int, int) __attribute__ ((regparm (2)));
// void
// foo (void)
{
  bar[0] (1, 2);
}
// [hjl@gnu-6 <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Array of function pointers with attribute leads to wrong code"
//    href="show_bug.cgi?id=68637">pr68637</a>]$ cat main.i
static void 
__attribute__ ((regparm (2)))
// bar0 (int i, int j)
{
  if (i != 1 || j != 2)
    __builtin_abort ();
}

typedef void (*func_t) (int, int) __attribute__ ((regparm (2)));

// func_t bar[] =
{
//   bar0,
};

extern void foo (void);
// int
// main ()
{
  foo ();
  return 0;
}
// [hjl@gnu-6 <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Array of function pointers with attribute leads to wrong code"
//    href="show_bug.cgi?id=68637">pr68637</a>]$ make CC=gcc x
// x.i:1:1: warning: ‘regparm’ attribute only applies to function types [-Wattributes]
 extern void (*bar[]) (int, int) __attribute__ ((regparm (2)));
//  ^
// [hjl@gnu-6 <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Array of function pointers with attribute leads to wrong code"
//    href="show_bug.cgi?id=68637">pr68637</a>]$ ./x
// Aborted
// [hjl@gnu-6 <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Array of function pointers with attribute leads to wrong code"
//    href="show_bug.cgi?id=68637">pr68637</a>]$


