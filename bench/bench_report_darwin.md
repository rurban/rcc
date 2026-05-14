# Darwin RCC Benchmark Results

_Generated: May 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           79 |          635 |        714 |
| RCC -O1   |           89 |          630 |        719 |
| TCC       |           57 |          559 |        616 |
| GCC -O0   |           76 |          474 |        550 |
| GCC -O2   |          118 |          282 |        400 |
| Clang -O0 |           74 |          475 |        549 |
| Clang -O2 |          100 |          286 |        386 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    498 us
  lex         bench.c:    140 us
  parse       bench.c:     60 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   2471 us
  peephole    bench.c:    237 us
  link        bench_rcc:  77237 us

RCC -O1:
  preprocess  bench.c:    398 us
  lex         bench.c:    145 us
  parse       bench.c:     55 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1739 us
  peephole    bench.c:    244 us
  link        bench_rcc_o1:  68175 us
```
