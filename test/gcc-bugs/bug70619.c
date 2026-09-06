/* GCC Bug #70619 - Wrong warning with VLA, comma and sizeof
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70619
 */
/* { dg-do compile } */


int main()
{
  int a[1][(0, 1)];
  int i = 0;
  sizeof a[i++];
}
