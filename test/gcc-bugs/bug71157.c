/* GCC Bug #71157 - -Wnull-dereference false alarm in wrong function
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71157
 */
/* { dg-do compile } */

// NOTE: the bugzilla report never reduced this to a standalone testcase.
// The reporter attached a ~10000-line preprocessed dump of GNU Emacs'
// lib-src/etags.c (attachment 38504) which triggers spurious
// -Wnull-dereference warnings (about skip_spaces()'s caller, and later
// about Forth_words/TeX_commands) at -O1/-O2/-O3 after inlining; even the
// GCC developers said "The testcase is too large for me to analyze
// further." No self-contained C snippet reproducing the false positive
// exists anywhere in the report, so this cannot be faithfully rendered as
// a single-TU reproducer; left as a compilable placeholder instead.
//
// Excerpt of the GIMPLE dump quoted in comment #5, showing the bogus path
// GCC's middle-end thinks may dereference a NULL pointer (skip_spaces()
// immediately dereferences its argument, so it cannot actually return
// NULL):
//
//   [pr71157.c:7841:12] # VUSE <.MEM_63>
//   _402 ={v} [pr71157.c:7841:12] MEM[(char *)0B];
//   # USE = nonlocal null { D.5991 } (nonlocal, escaped)
//   # CLB = nonlocal null { D.5991 } (nonlocal, escaped)
//   __builtin_trap ();
//
//   cp_201 = skip_spaces (_200);
//   if (cp_201 != 0B)
//     goto <bb 61>;
//   else
//     goto <bb 109>;
//
// The location shown is just an artifact of merging expressions and not
// preserving the right locations. The middle-end is not very smart at
// that, this is why middle-end warnings are often confusing.
// The testcase is too large for me to analyze further.


