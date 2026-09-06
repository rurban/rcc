/* GCC Bug #117866 - Confusing 'expected ... but argument is of type ...' (same type repeated)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117866
 */
/* { dg-do compile } */


struct XYspace {
       void (*convert)(struct fractpoint *pt);
} ;

struct fractpoint {
       double x,y;
} ;

struct segment {
       struct fractpoint dest;
} ;

void t1_Loc(register struct XYspace *S)
{
       register struct segment *r;
//        (*S->convert)(&r->dest);//, S, x, y);
}
// ```


