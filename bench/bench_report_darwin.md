# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          108 |          657 |        765 |
| RCC -O1   |          116 |          692 |        808 |
| TCC       |           76 |          607 |        683 |
| GCC -O0   |          171 |          553 |        724 |
| GCC -O2   |          205 |          301 |        506 |
| Clang -O0 |           83 |          531 |        614 |
| Clang -O2 |          142 |          293 |        435 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    939 us
  lex         bench.c:    105 us
  parse       bench.c:     82 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   3648 us
  peephole    bench.c:    593 us
  link        bench_rcc: 100495 us

RCC -O1:
  preprocess  bench.c:    909 us
  lex         bench.c:    103 us
  parse       bench.c:     58 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:   4072 us
  peephole    bench.c:    615 us
  link        bench_rcc_o1: 110890 us
```
