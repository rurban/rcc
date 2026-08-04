/* GCC Bug #21343 - incompatible internal linkage declarations in different scopes not diagnosed
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=21343
 */
/* { dg-do compile } */


static int (*a)[];
void f(void) { extern int (*a)[2]; }
void g(void) { extern int (*a)[3]; }

// Test 2 (where the prototype information from the inner scope should be checked
// against the old-style definition):

static int f(int (*)[]);
int g() { extern int f(int (*)[2]); }
static int f(a) int (*a)[3]; { return 0; }
// Test 3:
static int (*a)[];
void f(void) { extern int (*a)[]; extern int (*a)[2]; }
extern int (*a)[3];


