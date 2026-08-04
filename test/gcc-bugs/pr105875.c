/* GCC Bug #105875 - Toggling an atomic_bool is inefficient
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105875
 */


if (__atomic_compare_exchange_1 ((volatile void *) &b, (void *) &D.2819, (int) VIEW_CONVERT_EXPR<unsigned char>(D.2820), 0, 5, 5))
    {
      goto <D.2822>;
    }
  goto <D.2821>;
//   <D.2822>:;, D.2820;
vs:
  TARGET_EXPR <D.2827, (char) __atomic_xor_fetch_1 ((volatile void *) &c, (int) (unsigned char) TARGET_EXPR <D.2826, 1>, 5)>;, D.2827;
// So confirmed.
// Using __atomic_xor_fetch_1 directly works.
// That is:
  __atomic_xor_fetch_1 (&b, 1, 5);
Produces:
//         lock xorb       $1, b(%rip)


