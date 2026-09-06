/* GCC Bug #92392 - -Wignored-qualifiers points to wrong location and doesn't mention which qualifiers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92392
 */
/* { dg-do compile } */


typedef int int32_t;
static int32_t * const f1(void);

//     2 | static int32_t * const f1(void);
static int * const f1(void);

//     1 | static int * const f1(void);
static int32_t * const f1(void);


