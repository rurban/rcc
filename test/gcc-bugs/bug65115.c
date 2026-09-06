/* GCC Bug #65115 - default init_priority attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65115
 */
/* { dg-do compile } */


// From the GCC manual, @code{init_priority} variable attribute:
// "in a given translation unit.  No guarantee is made for initializations
// across translation units.  You can set the order in which the
// constructor is run by specifying an @code{init_priority} attribute by
// specifying a relative @var{priority}.
// In the following example, @code{A} would normally be created before
// @code{B}, but the @code{init_priority} attribute reverses that order:"
//
// The bug: the documentation does not describe the *default* priority
// used for objects that have no explicit init_priority attribute, nor
// what happens when some objects in a translation unit have priorities
// assigned and others do not.

struct Some_Class { int x; };

struct Some_Class Global_A __attribute__ ((init_priority (2000)));
struct Some_Class Global_B __attribute__ ((init_priority (543)));
struct Some_Class Global_C; /* no explicit priority: default order */

