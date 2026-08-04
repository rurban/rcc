/* GCC Bug #68039 - Incorrect unused-result warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68039
 */


__attribute__((warn_unused_result)) int x()
{
 return 0;
}

int main()
{
 return x() ? 0 : 0;
}
// <span class="quote">> gcc test.c</span >
// test.c: In function ‘main’:
// test.c:8:9: warning: ignoring return value of ‘x’, declared with attribute warn_unused_result [-Wunused-result]
  return x() ? 0 : 0;


