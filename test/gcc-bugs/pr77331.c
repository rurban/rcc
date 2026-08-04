/* GCC Bug #77331 - incorrect range location in -Wformat with a concatenated format literal
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77331
 */


extern int printf (const char*, ...);

void f (const char *msg)
{
  printf ("hello " "%i", msg);

#define INT_FMT "%i"

  printf ("hello " INT_FMT " world", msg);

}
// t.c: In function ‘f’:
// t.c:5:11: warning: format ‘%i’ expects argument of type ‘int’, but argument 2 has type ‘const char *’ [-Wformat=]
   printf ("hello " "%i", msg);
//            ^~~~~~~~
// t.c:5:22: note: format string is defined here
   printf ("hello " "%i", msg);
//                      ~^
//                      %s
// t.c:9:11: warning: format ‘%i’ expects argument of type ‘int’, but argument 2 has type ‘const char *’ [-Wformat=]
   printf ("hello " INT_FMT " world", msg);
//            ^~~~~~~~
// t.c:7:19: note: format string is defined here
 #define INT_FMT "%i"
//                   ~^
//                   %s


