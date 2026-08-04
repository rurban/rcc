/* GCC Bug #79775 - Confusing fix-it diagnostics with double pointers to structs
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79775
 */


struct s
    {
      int m;
    };

    void f(struct s **s)
    {
//       *s->m = 5;
//       // (*s)->m = 5;
    }
C:
//     ../f.c: In function 'f':
//     ../f.c:8:5: error: '*s' is a pointer; did you mean to use '->'?
//         8 |   *s->m = 5;
//           |     ^~
//           |     ->
// C++:
//     ../f.c: In function 'void f(s**)':
//     ../f.c:8:7: error: request for member 'm' in '* s', which is of pointer type 's*' (maybe you meant to use '->' ?)
//         8 |   *s->m = 5;
//           |       ^


