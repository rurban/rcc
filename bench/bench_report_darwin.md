# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           63 |          605 |        668 |
| RCC -O1   |           68 |          597 |        665 |
| TCC       |           35 |          515 |        550 |
| GCC -O0   |           63 |          434 |        497 |
| GCC -O2   |           88 |          262 |        350 |
| Clang -O0 |           62 |          436 |        498 |
| Clang -O2 |           88 |          265 |        353 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    415 us
  lex         bench.c:     96 us
  parse       bench.c:     55 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   1216 us
  peephole    bench.c:    226 us
  link        bench_rcc:  57369 us

RCC -O1:
  preprocess  bench.c:    432 us
  lex         bench.c:     96 us
  parse       bench.c:     51 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:   1300 us
  peephole    bench.c:    243 us
  link        bench_rcc_o1:  56392 us
```
