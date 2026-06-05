# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           72 |          647 |        719 |
| RCC -O1   |           94 |          642 |        736 |
| TCC       |           53 |          571 |        624 |
| GCC -O0   |          112 |          483 |        595 |
| GCC -O2   |          150 |          292 |        442 |
| Clang -O0 |           55 |          480 |        535 |
| Clang -O2 |           96 |          287 |        383 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    495 us
  lex         bench.c:    103 us
  parse       bench.c:     59 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1914 us
  peephole    bench.c:    235 us
  link        bench_rcc:  72472 us

RCC -O1:
  preprocess  bench.c:    574 us
  lex         bench.c:    105 us
  parse       bench.c:     55 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   2245 us
  peephole    bench.c:    236 us
  link        bench_rcc_o1:  70070 us
```
