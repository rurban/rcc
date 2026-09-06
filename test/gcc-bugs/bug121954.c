/* GCC Bug #121954 - Missing CLOBBER(eos) for temporary of function return/arg
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121954
 */
/* { dg-do compile } */


struct s1 {
  int i[1024];
};
void f1(struct s1 *);
static inline struct s1 f(struct s1 a)
{
  f1(&a);
  return a;
}

void h();

struct s1 g(int a)
{
  struct s1 b = {a};
  struct s1 c = f(f(f(f(b))));
  h();
  struct s1 s = f(f(f(f(c))));
  h();
  return s;
}
// ```
// The gimplifier produces:
// ```
//       D.4653 = f (b); [return slot optimization]
//       D.4654 = f (D.4653); [return slot optimization]
//       D.4655 = f (D.4654); [return slot optimization]
//       c = f (D.4655); [return slot optimization]
//       h ();
//       D.4656 = f (c); [return slot optimization]
//       D.4657 = f (D.4656); [return slot optimization]
//       D.4658 = f (D.4657); [return slot optimization]
//       <retval> = f (D.4658); [return slot optimization]
//       h ();
// ```
// But there is no clobber generated for the temporaries that was created from the function return/argument.
// Right now this produces:
// ```
// Partition 0: size 4096 align 8
// 	D.4658
// Partition 1: size 4096 align 8
// 	D.4657
// Partition 2: size 4096 align 8
// 	D.4656
// Partition 3: size 4096 align 8
// 	D.4655
// Partition 4: size 4096 align 8
// 	D.4654
// Partition 5: size 4096 align 8
// 	D.4653
// ```
// But there should be 3 paritions rather than 6.


