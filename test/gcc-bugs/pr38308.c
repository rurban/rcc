/* GCC Bug #38308 - -Wformat does not work for wide strings
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=38308
 */


int main()
{
  wprintf (L"%s", 5);
}

// GCC should try converting the string to single-byte (e.g. to UTF-8, which would work for any wchar_t encoding in which 0-127 maps to char's encoding) and test the format string.


