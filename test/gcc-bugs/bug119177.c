/* GCC Bug #119177 - target_clones with section vs alias attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119177
 */
/* { dg-do compile } */


static int 
__attribute__((target_clones("default,avx"))) 
__attribute__((__section__(".init.text"))) 
xt_init(void) {}

int init_module(void)
__attribute__((__copy__(xt_init)))
__attribute__((alias("xt_init")));
// ```


