/* GCC Bug #105430 - [DR 413] Change in value for "Partial overriding of constant struct/union initializers with designated initializers"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105430
 */
/* { dg-do compile } */


// 17 | const struct T t1 = { .s = s, .s.r.b = 5 };
//    17 | const struct T t1 = { .s = s, .s.r.b = 5 };
//    17 | const struct T t1 = { .s = s, .s.r.b = 5 };


