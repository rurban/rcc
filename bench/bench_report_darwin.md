# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           97 |          772 |        869 |
| RCC -O1   |          136 |          709 |        845 |
| TCC       |          111 |          637 |        748 |
| GCC -O0   |          166 |          531 |        697 |
| GCC -O2   |          140 |          367 |        507 |
| Clang -O0 |          112 |          572 |        684 |
| Clang -O2 |          123 |          350 |        473 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    598 us
  lex         bench.c:    106 us
  parse       bench.c:     57 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   3337 us
  peephole    bench.c:    388 us
  link        bench_rcc:  94123 us

RCC -O1:
  preprocess  bench.c:    553 us
  lex         bench.c:    106 us
  parse       bench.c:     59 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:   4555 us
  peephole    bench.c:    573 us
  link        bench_rcc_o1:  93970 us
```
