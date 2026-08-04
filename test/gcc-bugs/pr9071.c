/* GCC Bug #9071 - Warning for blocks not closed in same file as opened in
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=9071
 */
/* { dg-do compile } */


struct A
{
  int i;
#include "header.h"
int f(struct A *a)
{
//    a->i=0;
}


