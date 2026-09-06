/* GCC Bug #101290 - ICE with -O1 on valid code: in maybe_canonicalize_mem_ref_addr, at gimple-fold.c:5976
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101290
 */
/* { dg-do compile } */
/* { dg-options "-std=gnu89 -O1" } */

/* Reporter's mutant.c: deeply nested anonymous struct types whose size
 * overflows sizetype; ICE in maybe_canonicalize_mem_ref_addr at -O1.
 * Still reproduces with gcc 16 (gimple-fold.cc maybe_canonicalize_mem_ref_addr). */

typedef *a;
typedef struct {
  struct {
    struct {
      struct {
        struct {
          struct {
            struct {
              struct {
                struct {
                  struct {
                    struct {
                      struct {
                        struct {
                          struct {
                            struct {
                              struct {
                                struct {
                                  struct {
                                    struct {
                                      struct {
                                        struct {
                                          struct {
                                            struct {
                                              struct {
                                                struct {
                                                  struct {
                                                    struct {
                                                      struct {
                                                        unsigned b, c, d, e
                                                          } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                                    } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                                  } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                                } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                              } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                            } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                          } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                        } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                      } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                    } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                  } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                                } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                              } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                            } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                          } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                        } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                      } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                    } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                  } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
                } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
              } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
            } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
          } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
        } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
      } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
    } b, c, d, e, f /* { dg-warning "no semicolon at end of struct or union" } */
  } b, c, f /* { dg-warning "no semicolon at end of struct or union" } */
} * g;
int h;
struct i k;
struct i {
  a j
} l(struct i *m) {
  *(volatile *)&((g)m->j)->f;
}
n() {
  k.j = &h;
  l(&k);
}
