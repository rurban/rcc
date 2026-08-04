/* GCC Bug #51834 - -Wsequence-point fails when convoluted expressions with multiple side effects are used
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51834
 */


int i, j;

static inline int set_flag (void)
{
  j |= 1;
  return 0;
}

#define FOO (i ? (j |= 1, 0) : 0)
#define BAR (i ? set_flag () : 0)

void fct (void)
{
  FOO || FOO;
  FOO | FOO;
  BAR | BAR;
  set_flag () + set_flag ();
  j = (++i, j) + (j, ++i);
//   return;
}

// GCC 4.7.0 warns only for "FOO | FOO;" (and I think that's incorrect, as said above).


