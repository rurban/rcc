# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          117 |          727 |        844 |
| RCC -O1   |          105 |          772 |        877 |
| TCC       |          118 |          732 |        850 |
| GCC -O0   |          147 |          607 |        754 |
| GCC -O2   |          176 |          350 |        526 |
| Clang -O0 |          109 |          608 |        717 |
| Clang -O2 |          175 |          314 |        489 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    709 us
  lex         bench.c:    108 us
  parse       bench.c:     86 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   5832 us
  peephole    bench.c:    391 us
  link        bench_rcc: 104622 us

RCC -O1:
  preprocess  bench.c:    466 us
  lex         bench.c:    105 us
  parse       bench.c:     59 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:   3748 us
  peephole    bench.c:    492 us
  link        bench_rcc_o1: 103890 us
```
