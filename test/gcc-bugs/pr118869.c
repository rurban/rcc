/* GCC Bug #118869 - infinite recusion in gimplifier with __builtin_assoc_barrier and structs
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=118869
 */


typedef __SIZE_TYPE__ size_t;

struct S1 {
  unsigned char pad1;
  unsigned char val;
};

extern unsigned char t[256];

struct S1 foo(struct S1 a, size_t i) {
//   a.val = t[i];

  return __builtin_assoc_barrier(a);
}
// ```


