# Darwin RCC Benchmark Results

_Generated: May 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           90 |          604 |        694 |
| RCC -O1   |           70 |          601 |        671 |
| TCC       |           37 |          531 |        568 |
| GCC -O0   |           68 |          451 |        519 |
| GCC -O2   |          130 |          272 |        402 |
| Clang -O0 |           54 |          448 |        502 |
| Clang -O2 |           79 |          271 |        350 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    492 us
  lex         bench.c:     68 us
  parse       bench.c:     52 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   1402 us
  peephole    bench.c:    224 us
  link        bench_rcc:  67557 us

RCC -O1:
  preprocess  bench.c:    639 us
  lex         bench.c:     71 us
  parse       bench.c:     51 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   2556 us
  peephole    bench.c:    257 us
  link        bench_rcc_o1:  76642 us
```
