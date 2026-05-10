# Darwin RCC Benchmark Results

_Generated: May 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          129 |          711 |        840 |
| RCC -O1   |          176 |          789 |        965 |
| TCC       |           52 |          644 |        696 |
| GCC -O0   |          211 |          594 |        805 |
| GCC -O2   |          150 |          323 |        473 |
| Clang -O0 |          100 |          470 |        570 |
| Clang -O2 |           89 |          295 |        384 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    670 us
  lex         bench.c:     73 us
  parse       bench.c:     51 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   7788 us
  peephole    bench.c:   1293 us
  link        bench_rcc: 117005 us

RCC -O1:
  preprocess  bench.c:    505 us
  lex         bench.c:     70 us
  parse       bench.c:     54 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1802 us
  peephole    bench.c:    233 us
  link        bench_rcc_o1:  97164 us
```
