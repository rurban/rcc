/* GCC Bug #53277 - -Wconversion cannot handle compound expressions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53277
 */


({ char __a0, __a1, __a2;                                                   \
      : __builtin_strspn (s, accept)); })


