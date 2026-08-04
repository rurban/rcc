/* GCC Bug #39438 - Can't compile a wrapper around strftime with -Werror=format-nonliteral
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39438
 */


static void __attribute__((format (strftime, 1, 0)))
// test1(const char *fmt, const struct tm *tm)
{
 char buf[100];

// 	strftime(buf, sizeof(buf), fmt, tm);
}

static const char *__attribute__((format_arg (1)))
// helper(const char *fmt)
{
 return fmt;
}

static void
// test2(const char *fmt, const struct tm *tm)
{
 char buf[100];

// 	strftime(buf, sizeof(buf), helper(fmt), tm);
}


