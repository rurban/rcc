# Darwin RCC Benchmark Results

_Generated: May 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           92 |          719 |        811 |
| RCC -O1   |           81 |          719 |        800 |
| TCC       |           66 |          657 |        723 |
| GCC -O0   |          133 |          572 |        705 |
| GCC -O2   |          169 |          317 |        486 |
| Clang -O0 |           72 |          522 |        594 |
| Clang -O2 |          110 |          318 |        428 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    474 us
  lex         bench.c:    135 us
  parse       bench.c:     58 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   1880 us
  peephole    bench.c:    228 us
  link        bench_rcc:  79897 us

RCC -O1:
  preprocess  bench.c:    715 us
  lex         bench.c:    140 us
  parse       bench.c:     59 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:   3208 us
  peephole    bench.c:    364 us
  link        bench_rcc_o1:  94982 us
```
