/* GCC Bug #91672 - wrong amount of storage allocated for initialized structs with flexible array members
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91672
 */
/* { dg-do compile } */


struct A
{
  __INT64_TYPE__ i64;
  __INT16_TYPE__ i16;
  __INT16_TYPE__ a16[];
};

struct A a0 = { 0, 1 };
struct A a1 = { 1, 1, { 1 } };
struct A a2 = { 2, 1, { 1, 2 } };
struct A a3 = { 3, 1, { 1, 2, 3 } };
struct A a4 = { 4, 1, { 1, 2, 3, 4 } };


