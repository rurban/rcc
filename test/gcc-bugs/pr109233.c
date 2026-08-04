/* GCC Bug #109233 - warning: array subscript 5 is above array bounds of ‘struct tg3_napi[5]’ since r12-2591
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109233
 */


struct S { unsigned x, y, z; };
struct T { struct S f[5]; unsigned h; };
void foo (void);
// void
// bar (struct T *t)
{
  for (int i = 0; i < t->h; i++)
    {
      struct S *s = &t->f[i]; /* { dg-bogus "array subscript 5 is above array bounds" } */
      if (i <= 4)
// 	s->y = 1;
//       s->z = 2;
      if (i)
//         s->x = 3;
    }
}
// I guess what is going on is that we thread the body,
  if (i > 4)
    {
//       s->z = 2;
      goto do_x;
    }
//   else
    {
//       s->y = 1;
//       s->z = 2;
      if (i)
        {
        do_x:
//           s->x = 3;
        }
    }
// because if i > 4, we know we don't want to store s->y and know we don't need to check if i is non-zero further.
// Next evrp determines that the range of the i_4 index is [0, 5] for some reason rather than the [0, 4] for which it is well defined, perhaps because of the dead
  s_15 = &t_10(D)->f[i_4];
// statement that nothing has DCEd yet or what, and everything goes wrong from that point,
// as evrp because of that folds the
  MEM <struct T> [(struct S *)t_10(D)].f[i_4].z = 2;
// statement done only for i_4 > 4 into
  MEM <struct T> [(struct S *)t_10(D)].f[5].z = 2;
// and later we warn on that very statement.
// Now, a question on the kernel side is obviously why when
#define TG3_RSS_MAX_NUM_QS              4
#define TG3_IRQ_MAX_VECS_RSS            (TG3_RSS_MAX_NUM_QS + 1)
#define TG3_IRQ_MAX_VECS                TG3_IRQ_MAX_VECS_RSS
// ...
        struct tg3_napi                 napi[TG3_IRQ_MAX_VECS];
// it has the
        for (i = 0; i < tp->irq_max; i++) {
                struct tg3_napi *tnapi = &tp->napi[i];

//                 tnapi->tp = tp;
//                 tnapi->tx_pending = TG3_DEF_TX_RING_PENDING;

//                 tnapi->int_mbox = intmbx;
                if (i <= 4)
                        intmbx += 0x8;
//                 else
                        intmbx += 0x4;
// rather than just doing intmbx == 0x8; always.  That introduction of the dead code there confuses the warning.

// And on the ranger side why we have determined the [0, 5] range rather than [0, 4], whether it is related to inaccurate number of iterations estimation, or ranger using it incorrectly, ...


