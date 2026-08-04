/* GCC Bug #111816 - [gimple FE] ICE with _GIMPLE(ssa) and 2 returns
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111816
 */
/* { dg-do compile } */


{
//           edge e = make_edge (BASIC_BLOCK_FOR_FN (cfun, parser.edges[i].src),
//                               BASIC_BLOCK_FOR_FN (cfun, parser.edges[i].dest),
//                               parser.edges[i].flags);
//           e->probability = parser.edges[i].probability;
        }


