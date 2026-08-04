/* GCC Bug #23577 - spurious warnings about unhandled cases in switches (need VRP and control flow in front-end)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=23577
 */
/* { dg-do compile } */


typedef enum { FOO, BAR, BAZ } foo_t;

int foo(foo_t v)
{
    switch(v)
    {
    case FOO:
 return 0;

    default:
 break;
    }

    switch(v)
    {
    case BAR:
 break;

    case BAZ:
 break;
   } 
   return 1;
}
// ---end example program---


