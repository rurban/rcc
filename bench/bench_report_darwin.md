# Darwin RCC Benchmark Results

_Generated: May 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           64 |          580 |        644 |
| RCC -O1   |           66 |          581 |        647 |
| TCC       |           35 |          512 |        547 |
| GCC -O0   |           65 |          433 |        498 |
| GCC -O2   |           84 |          264 |        348 |
| Clang -O0 |           56 |          432 |        488 |
| Clang -O2 |           74 |          261 |        335 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    453 us
  lex         bench.c:     97 us
  parse       bench.c:     54 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1265 us
  peephole    bench.c:    281 us
  link        bench_rcc:  63420 us

RCC -O1:
  preprocess  bench.c:    436 us
  lex         bench.c:     98 us
  parse       bench.c:     51 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1438 us
  peephole    bench.c:    229 us
  link        bench_rcc_o1:  63021 us
```
