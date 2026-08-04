/* GCC Bug #37591 - suppress "signed and unsigned" warnings when signed value known to be positive
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37591
 */


#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

unsigned int
// constrain(unsigned int index, unsigned int offset, unsigned int limit)
{
  int adj = index - offset;
  adj = MAX(adj, 0);
//   return MIN(adj, limit); /* { dg-bogus "signed and unsigned" } */
}


