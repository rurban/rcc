/* GCC Bug #120380 - internal compiler error: error reporting routines re-entered
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=120380
 */
/* { dg-do compile } */

/* The official gcc.dg/pr120380.c testcase from the fix commit
 * (r16-1036, c: fix ICE related to tagged types with attributes in
 * diagnostics).  The ICE was in get_aka_type re-entering warning code;
 * gcc now reports the nested-redefinition errors cleanly. */
struct pair_t {
  char c;
  __int128_t i;
} __attribute__((packed));
typedef struct unaligned_int128_t_ {
  struct unaligned_int128_t_ { /* { dg-error "nested redefinition of .struct unaligned_int128_t_." } */
    struct unaligned_int128__{
      __int128_t value;
    }
  }
} __attribute__((packed, may_alias)) unaligned_int128_t;
struct pair_t p = {0, 1};
unaligned_int128_t *addr = (unaligned_int128_t *)&p.i;
int main() {
  addr->value = 0; /* { dg-error "has no member named .value." } */
  return 0;
}
