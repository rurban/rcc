# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           71 |          645 |        716 |
| RCC -O1   |           89 |          663 |        752 |
| TCC       |           49 |          580 |        629 |
| GCC -O0   |          105 |          491 |        596 |
| GCC -O2   |          103 |          294 |        397 |
| Clang -O0 |           78 |          480 |        558 |
| Clang -O2 |          112 |          298 |        410 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    492 us
  lex         bench.c:    102 us
  parse       bench.c:     55 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   2428 us
  peephole    bench.c:    236 us
  link        bench_rcc:  69360 us

RCC -O1:
  preprocess  bench.c:    384 us
  lex         bench.c:    100 us
  parse       bench.c:     53 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   2774 us
  peephole    bench.c:    238 us
  link        bench_rcc_o1:  70004 us
```
