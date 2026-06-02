# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          109 |          772 |        881 |
| RCC -O1   |          122 |          772 |        894 |
| TCC       |          116 |          649 |        765 |
| GCC -O0   |          121 |          538 |        659 |
| GCC -O2   |          203 |          346 |        549 |
| Clang -O0 |           95 |          588 |        683 |
| Clang -O2 |          127 |          309 |        436 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    944 us
  lex         bench.c:    109 us
  parse       bench.c:     62 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   3192 us
  peephole    bench.c:    394 us
  link        bench_rcc:  93518 us

RCC -O1:
  preprocess  bench.c:    575 us
  lex         bench.c:    140 us
  parse       bench.c:     57 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:   2557 us
  peephole    bench.c:    265 us
  link        bench_rcc_o1:  83563 us
```
