/* GCC Bug #123370 - ICE: in build_conditional_expr, at c/c-typeck.cc with _Float16 and ternary operator and !
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123370
 */
/* { dg-do compile } */


typedef _Float16 TFtype;

int main() {
    TFtype x = (TFtype)3.14;
    TFtype y = (TFtype)1.0;
    int n = 0;

    return (y == x) ? (y + n) : !(x - y);
}

//    25 |     return y == x ? y + n : !(x - y);
// 0x569ce3c internal_error(char const*, ...)
// 0x563a4de fancy_abort(char const*, int, char const*)
// 0xeaf93c build_conditional_expr(unsigned long, tree_node*, bool, tree_node*, tree_node*, unsigned long, tree_node*, tree_node*, unsigned long)
// 0xfd4dac c_parse_file()
// 0x1106e32 c_common_parse_file()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


