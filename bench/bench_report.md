# Linux RCC Benchmark Results

_Generated: May 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           65 |          676 |        741 |
| RCC -O1   |           73 |          698 |        771 |
| TCC       |           18 |          687 |        705 |
| SLIMCC    |           56 |          693 |        749 |
| KEFIR     |          261 |          772 |       1033 |
| KEFIR -O1 |          237 |          543 |        780 |
| GCC -O0   |           94 |          626 |        720 |
| GCC -O2   |          212 |          234 |        446 |
| Clang -O0 |          115 |          741 |        856 |
| Clang -O2 |          171 |          247 |        418 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    418 us
  lex         bench.c:    196 us
  parse       bench.c:    363 us
  typecheck   bench.c:      9 us
  codegen     bench.c:   1103 us
  peephole    bench.c:    667 us
  link        bench_rcc:  68756 us

RCC -O1:
  preprocess  bench.c:   1239 us
  lex         bench.c:    299 us
  parse       bench.c:    648 us
  typecheck   bench.c:     25 us
  opt(CTFE)   bench.c:     61 us
  codegen     bench.c:   1559 us
  peephole    bench.c:    848 us
  link        bench_rcc_o1:  53930 us
```
