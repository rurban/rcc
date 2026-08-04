/* GCC Bug #87879 - -Wformat-nonliteral could see more things as literals
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87879
 */


#include <stdio.h>
#include <string.h>
const char *foo (const char *fmt, ...)
  __attribute__((format(printf, 1, 2)));

int f1(const char *s, int x)
{
    const char *fmt = "s: %s, x: %d\n";
//     foo(fmt, s, x);
    return strlen(fmt);
}
int f2(const char *s, int x)
{
    const char *fmt = "s: %d, x: %s\n";
//     foo(fmt, s, x);
    return strlen(fmt);
}
int g1(const char *s, int x)
{
    const char *const fmt = "ss: %s, xx: %d\n";
//     foo(fmt, s, x);
    return strlen(fmt);
}
int g2(const char *s, int x)
{
    const char *const fmt = "ss: %d, xx: %s\n";
//     foo(fmt, s, x);
    return strlen(fmt);
}

// With -Wformat-nonliteral, gcc diagnoses f1 and f2, which is of course not strictly wrong. However, since fmt is never assigned to apart from its initialization (and its address is not taken and passed on etc.), gcc should be able to deduce the constness of fmt and act as for g1 and g2 (granted, that only works at -O1 and higher). What's worse, -Wformat-nonliteral is not part of -Wall -Wextra, so with "just" -Wall -Wextra, f2() isn't diagnosed at all.

// The strlen() calls are just there to show that gcc does fold those to constants in all cases (at -O1 and higher). So this is of course dependent on optimization passes, and the format checking presumably happens quite early.

// Neverthess, the f1() case is rather common. Is there some magic one could insert above

  if (TREE_CODE (format_tree) != ADDR_EXPR)
    {
//       res->number_non_literal++;
//       return;
    }

// to replace format_tree with its DECL_INITIAL in a case like that, to silence Wformat-nonliteral and allow diagnosing f2()?


