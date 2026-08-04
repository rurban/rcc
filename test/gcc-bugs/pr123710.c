/* GCC Bug #123710 - ICE (in  default_conversion) with invalid argument to vector_size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123710
 */
/* { dg-do compile } */


typedef unsigned long long
    __attribute__((vector_size(*(int (*)())0xbabebec0))) V2DI;
// ```


