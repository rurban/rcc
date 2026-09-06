/* GCC Bug #80592 - gcc fails to detect overflow in shift statement
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80592
 */


#define TU_SIZE_RECOMMENDED     (0x3f << 16) 

void g( int a)
{
  __builtin_printf ("value: %d\n", a);
}

int main()
{
    int max_tu_symbol = TU_SIZE_RECOMMENDED - 1;

//     g(max_tu_symbol << 23);

    return 0;
}
// g++ -O2 -fsanitize=undefined <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - gcc fails to detect overflow in shift statement"
//    href="show_bug.cgi?id=80592">pr80592</a>.c && ./a.out 
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - gcc fails to detect overflow in shift statement"
//    href="show_bug.cgi?id=80592">pr80592</a>.c:12:21: runtime error: left shift of 4128767 by 23 places cannot be represented in type 'int'
// value: -8388608


