/* GCC Bug #99577 - Non-constant (but actually constant) initializers referencing other constants no longer diagnosed as of GCC 8
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99577
 */


const int i = 0;
  const int j = i;
 const int j = i;

// As in the similar (and perhaps related?) <a class="bz_bug_link


