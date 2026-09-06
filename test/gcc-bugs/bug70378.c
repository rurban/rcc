/* GCC Bug #70378 - wrong warning with -Wconversion with explicit cast
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70378
 */
/* { dg-do compile } */


typedef unsigned int uint32_t;
void foo(char a, uint32_t b)
{
 b = (uint32_t)((b * 10) + (uint32_t)a); 
}
// Something must be removing the explicit cast or messing up the expression.


