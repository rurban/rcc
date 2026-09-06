/* GCC Bug #85949 - __attribute__ ((format (printf,1,1)));  improve error messages
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85949
 */


void str_fmt(const char * const format, ...) __attribute__ ((format (printf,1,2)));

int main()
{
    const int i = 0;
    const char * str = "abc";
    const char * str2 = "%s";

//     str_fmt(str, i);

//     str_fmt(str2, i);
}
// t.c: In function ‘main’:
// t.c:11:5: warning: ‘%s’ directive argument is null [-Wformat-overflow=]
//    11 |     str_fmt(str2, i);
//       |     ^~~~~~~~~~~~~~~~


