/* GCC Bug #116082 - -Wunterminated-string-initialization should not warn about strings that end with "\0" but should warn still for -Wc++-compat
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116082
 */
/* { dg-do compile } */


// 170 |         {a_X(a_VEXPR_CMD_AGN_DATE_STAMP_UTC, 0), "date-stamp-utc\0"},


