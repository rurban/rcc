/* GCC Bug #109233 - warning: array subscript 5 is above array bounds of ‘struct tg3_napi[5]’ since r12-2591
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109233
 */
/* { dg-do compile } */
/* { dg-options "-O2 -Warray-bounds" } */

/* Reduction of the kernel tg3.c pattern: gcc -O2 threads the body and
 * ranger derives the wrong [0,5] range for the index, producing a
 * spurious "array subscript 5 is above array bounds" on valid code. */
struct S { unsigned x, y, z; };
struct T { struct S f[5]; unsigned h; };
void foo (void);
void bar (struct T *t)
{
  for (int i = 0; i < t->h; i++)
    {
      struct S *s = &t->f[i];
      if (i <= 4)
        s->y = 1;
      s->z = 2;
      if (i)
        s->x = 3;
      if (i > 4)
        {
          s->z = 2;
          goto do_x;
        }
      else
        {
          s->y = 1;
          s->z = 2;
          if (i)
            {
            do_x:
              s->x = 3;
            }
        }
    }
}