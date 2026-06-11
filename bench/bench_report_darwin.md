# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           87 |          749 |        836 |
| RCC -O1   |          118 |          697 |        815 |
| TCC       |           89 |          676 |        765 |
| GCC -O0   |          130 |          593 |        723 |
| GCC -O2   |          194 |          333 |        527 |
| Clang -O0 |           84 |          630 |        714 |
| Clang -O2 |          184 |          332 |        516 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    574 us
  lex         bench.c:    103 us
  parse       bench.c:     57 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   2573 us
  peephole    bench.c:    241 us
  link        bench_rcc:  73969 us

RCC -O1:
  preprocess  bench.c:    449 us
  lex         bench.c:    103 us
  parse       bench.c:     58 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:   2877 us
  peephole    bench.c:    296 us
  link        bench_rcc_o1:  73636 us
```
