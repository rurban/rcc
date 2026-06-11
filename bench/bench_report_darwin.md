# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           82 |          647 |        729 |
| RCC -O1   |          102 |          665 |        767 |
| TCC       |           82 |          646 |        728 |
| GCC -O0   |          135 |          526 |        661 |
| GCC -O2   |          123 |          305 |        428 |
| Clang -O0 |          105 |          531 |        636 |
| Clang -O2 |          112 |          315 |        427 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    596 us
  lex         bench.c:    106 us
  parse       bench.c:     59 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   2475 us
  peephole    bench.c:    318 us
  link        bench_rcc:  74219 us

RCC -O1:
  preprocess  bench.c:    463 us
  lex         bench.c:    101 us
  parse       bench.c:     57 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:   2791 us
  peephole    bench.c:    240 us
  link        bench_rcc_o1:  65569 us
```
