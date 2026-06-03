# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           78 |          653 |        731 |
| RCC -O1   |          136 |          697 |        833 |
| TCC       |           66 |          587 |        653 |
| GCC -O0   |          109 |          559 |        668 |
| GCC -O2   |          123 |          361 |        484 |
| Clang -O0 |          129 |          640 |        769 |
| Clang -O2 |          164 |          360 |        524 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    389 us
  lex         bench.c:     99 us
  parse       bench.c:     60 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1561 us
  peephole    bench.c:    229 us
  link        bench_rcc:  78836 us

RCC -O1:
  preprocess  bench.c:    485 us
  lex         bench.c:    106 us
  parse       bench.c:     58 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     48 us
  codegen     bench.c:   1511 us
  peephole    bench.c:    257 us
  link        bench_rcc_o1:  66782 us
```
