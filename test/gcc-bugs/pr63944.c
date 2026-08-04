/* GCC Bug #63944 - [DR413] Partial overriding of nonconstant struct/union initializers with designated initializers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63944
 */


extern void exit (int);
extern void abort (void);
typedef struct {
int k;
int l;
int a[2];
} T;

typedef struct {
int i;
T t;
} S;

T x = {.l = 43, .k = 42, .a[1] = 19, .a[0] = 18 };
// int
// main (void)
{
S l = { 1, .t = x, .t.l = 41, .t.a[1] = 17};
if (l.t.k == 42) exit (0); else abort ();
}

// Clearly the existing approach for string constants (of splitting them individual characters) won't work here (but is still needed for initializers of objects with static storage duration).  This bug is specific to initializers of objects with automatic storage duration; it can apply both to initialization with an expression of struct type, and to initialization with one of union type (in the union case, the override would have to be of just part of a union member, as setting the whole of a union member sets the whole union).  I suppose some initializations with such partial overriding need to generate more complicated code - one possibility might be for such partial overriding to replace the initializer for .t with (tmp = x, tmp.l = 41, tmp) for a new temporary variable tmp.


