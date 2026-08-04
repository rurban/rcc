/* GCC Bug #90941 - [rfe] attribute to specify write-once static variable for early-initialized values
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90941
 */
/* { dg-do compile } */


static int priv_offset;

  struct offset_t { const int* const restrict p; };

  static inline offset_t
//   read_offset (void)
  {
    static struct offset_t x { &priv_offset };
    return x;
  }

  #define pub_offset (*read_offset ())

// These declarations make it possible to modify priv_offset only until the first time pub_offset has been used to call read_offset().  After that, modifying priv_offset is undefined because a const qualified restricted pointer to it exists.

// The code doesn't do anything useful in GCC.  It optimizes calls to read_offset() into multiple reads of priv_offset, defeating the guarantee.


