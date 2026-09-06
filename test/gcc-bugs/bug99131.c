/* GCC Bug #99131 - gcc doesn't detect missing comma in array initialisation [-Wstring-concatenation]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99131
 */
/* { dg-do compile } */


const char * a[ 4] = {
};
// feb17f.cc:7:2: warning: suspicious concatenation of string literals in an array initialization; did you mean to separate the elements with a comma? [-Wstring-concatenation]


