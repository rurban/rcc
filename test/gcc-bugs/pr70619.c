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
// gives such warnings:
// example.c: In function ‘main’:
// example.c:3:14: warning: left-hand operand of comma expression has no effect [-Wunused-value]
   int a[1][(0, 1)];
//               ^
// example.c:5:10: warning: right-hand operand of comma expression has no effect [-Wunused-value]
   sizeof a[i++];
//    ~~~~~~~^~~~~~
// The first one is fine but the second one is wrong and confusing.


