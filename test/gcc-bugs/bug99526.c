/* GCC Bug #99526 - Casts should retain typedef information
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99526
 */
/* { dg-do compile } */


typedef int i;
typedef const int cint;
typedef const i ci;
char v;

// Currently, any cast in C produces the "resolved" type regardless of
// whether the cast is written using a typedef.  E.g. (informal notation,
// "typeof(expr) => T" meaning "typeof(expr) is T"):
//
//   typeof((i)v)        => int;
//   typeof((cint)v)     => int;
//   typeof((ci)v)       => int;
//   typeof((const i)v)  => int;
//
// The IMHO ideal (still unimplemented) situation would be:
//
//   typeof((i)v)        => i;
//   typeof((cint)v)     => int;
//   typeof((ci)v)       => i;
//   typeof((const i)v)  => i;
//
// Note that since casts produce rvalues, qualifiers need to be stripped,
// and thus "ci" and "cint" shouldn't be used since the typedef itself
// contains a qualifier.
//
// NOTE: this is a request to retain typedef *names* purely for internal
// diagnostics/plugin purposes; it is not observable via the type system
// (a typedef and its underlying type remain compatible types either way),
// so there is no dg-error/dg-warning single-TU check that can distinguish
// current from desired behavior here.
//


