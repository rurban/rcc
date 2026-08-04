/* GCC Bug #53924 - unhelpful diagnostic in invalid declaration list
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53924
 */


typedef void *tree;
tree klass, tree cdecl, class_array_type;
// t.c:2:18: error: expected '=', ',', ';', 'asm' or '__attribute__' before 'cdecl'
 tree klass, tree cdecl, class_array_type;
//                   ^
// $
// The parser should be able to recover from this error more gracefully.


