/* GCC Bug #37041 - [meta-bug] -Wc++-compat refinements
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37041
 */
/* { dg-do compile } */


#ifdef __cplusplus
extern "C" {
#endif

/* code that is C-only */
int and(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
// I would expect -Wc++-compat to treat something like the above similarly to how the following pragmas would work:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"

// /* code that -Wc++-compat should ignore */

#pragma GCC diagnostic pop
// While the version with pragmas is shorter, the `extern "C"` stuff is often needed for separate reasons, and as such, it would be nice to avoid the need to do both...
// 2. Make -Wc++-compat an "umbrella" warning flag like -Wall or -Wextra. Currently -Wc++-compat warns about many different things, and sometimes I would like to just focus on one of those things. I can think of at least the following sub-flags that could be split off from it:
// ** "-Wc++-compat-pointer-conversion" for the ones like "warning: request for implicit conversion from 'void *' to 'char *' not permitted in C++"
// ** "-Wc++-compat-keywords" for the ones like "warning: identifier 'class' conflicts with C++ keyword"
// ** "-Wc++-compat-visibility" for the ones like "warning: enum constant defined in struct or union is not visible in C++" (or likewise with "struct" or "union" instead of "enum constant")


