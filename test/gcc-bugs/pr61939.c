/* GCC Bug #61939 - warn when __attribute__((aligned(x))) is ignored
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61939
 */
/* { dg-do compile } */


struct some_struct { int foo; };

void copy_something(void *p, const void *s) {
    struct some_struct __attribute__((aligned(8))) *_d = p;
    struct some_struct __attribute__((aligned(8))) *_s = s;
    *_d = *_s;
}

// It would seem to me that gcc should issue a warning when such an
// attribute is assigned, but has no effect as it does in some other
// cases. This seems to apply to all cases where it is used to define a
// type, of which you derive a pointer, e.g.:
//
//   int __attribute__((aligned(1))) *i;
//
// Here, I am actually expecting to get a pointer that I can safely access
// ints that are not aligned to the machine word, but will indeed blow up
// on machines that do not allow unaligned access of words. The quandary
// here is that simply treating it as an array of bytes is less efficient
// on x86, where an unaligned 32-bit mov would be faster than a rep movsb,
// so such a request is often highly reasonable.
//
// The proper way to tell gcc about assumed alignment is via
// __builtin_assume_aligned(), which returns the (possibly re-typed)
// pointer whose return value must be used:
//
//   void copy_something2(void *p, const void *s) {
//       struct some_struct *_d = __builtin_assume_aligned(p, 8);
//       struct some_struct *_s = __builtin_assume_aligned(s, 8);
//       *_d = *_s;
//   }
//
// (comment 3, egallager) had to modify the original reporter's testcase a
// bit to get it to actually compile (undeclared struct, non-standard
// __aligned__ spelling); that reduced testcase is what is used above.


