/* GCC Bug #70952 - Missing warning for likely-erroneous octal escapes in string literals [-Woctal-escapes]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70952
 */
/* { dg-do compile } */


const char s[] = "\008";

// (just the two zeros following the backslash become a part of the octal literal, so the string literal is equivalent to "\0""8")

// \008 and \009 in string literals are most likely errors (\08 and \09 work just as well if a nil character followed by a digit was really intended)

// I think a bit of a bikeshed is possible on the point how far we want to take it (do we warn for "\08"? for "\799"?).  I think warning when the octal escape with less than 3 digits is followed by [89] is desirable (this catches all of the above), but warning when octal escape already has 3 digits may be not (this exempts "\0009" from the warning).


