# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          127 |          767 |        894 |
| RCC -O1   |          190 |          736 |        926 |
| TCC       |           99 |          667 |        766 |
| GCC -O0   |          116 |          584 |        700 |
| GCC -O2   |          174 |          347 |        521 |
| Clang -O0 |          120 |          610 |        730 |
| Clang -O2 |          115 |          366 |        481 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    511 us
  lex         bench.c:    103 us
  parse       bench.c:     56 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   2387 us
  peephole    bench.c:    261 us
  link        bench_rcc:  88860 us

RCC -O1:
  preprocess  bench.c:    610 us
  lex         bench.c:    103 us
  parse       bench.c:     58 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:   2413 us
  peephole    bench.c:    233 us
  link        bench_rcc_o1:  91591 us
```
