# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           88 |          651 |        739 |
| RCC -O1   |          152 |          744 |        896 |
| TCC       |          112 |          642 |        754 |
| GCC -O0   |          163 |          582 |        745 |
| GCC -O2   |          126 |          302 |        428 |
| Clang -O0 |          133 |          499 |        632 |
| Clang -O2 |          102 |          300 |        402 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    544 us
  lex         bench.c:    101 us
  parse       bench.c:     59 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   4692 us
  peephole    bench.c:    356 us
  link        bench_rcc: 111774 us

RCC -O1:
  preprocess  bench.c:    467 us
  lex         bench.c:    108 us
  parse       bench.c:     61 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:   2718 us
  peephole    bench.c:    245 us
  link        bench_rcc_o1:  61119 us
```
