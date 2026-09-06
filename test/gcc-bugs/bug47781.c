/* GCC Bug #47781 - warnings from custom printf format specifiers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47781
 */
/* { dg-options "-std=gnu17 -w" } */

#include <stdio.h>
#include <printf.h>

int my_func (FILE *stream, const struct printf_info *info,
	     const void *const *args);
int my_arginfo (const struct printf_info *info, size_t n, int *argtypes,
		 int *size);

typedef int bool;

int
main (void)
{
  bool foo = 1;
  bool bar = 0;
  register_printf_specifier ('b', my_func, my_arginfo);
  printf ("true bool: %b  false bool: %b\n", foo, bar);
  return 0;
}

int
my_func (FILE *stream, const struct printf_info *info,
	 const void *const *args)
{
  int rv = 0;
  int this = *(int *) args[0];
  rv = fprintf (stream, "%s", this ? "TRUE" : "FALSE");
  return rv;
}

int
my_arginfo (const struct printf_info *info, size_t n, int *argtypes,
	    int *size)
{
  /* We expect 1 argument. */
  argtypes[0] = PA_INT;
  size[0] = sizeof (int);
  return 1;
}

// This is the exact reproducer attached to the bug report (attachment
// 23380), demonstrating a custom "%b" printf conversion registered via
// glibc's register_printf_specifier().  Compiled with -Wall (no -w), gcc
// reports:
// 47781.c: In function 'main':
// 47781.c:12: warning: unknown conversion type character 'b' in format
// 47781.c:12: warning: unknown conversion type character 'b' in format
// 47781.c:12: warning: too many arguments for format
// even though the specifier was legitimately registered and handled at
// runtime (the program still prints "true bool: TRUE  false bool: FALSE").
// The feature request (still open) is a way to teach GCC's -Wformat
// checking about custom printf specifiers - e.g. via a
// printf_format_specifier attribute (comment 32) or a simpler
// -Wno-format-unknown-specifier option (comment 0) - instead of having to
// disable -Wformat entirely.
