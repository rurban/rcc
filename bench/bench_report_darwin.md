# Darwin RCC Benchmark Results

_Generated: May 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          159 |          608 |        767 |
| RCC -O1   |           89 |          642 |        731 |
| TCC       |           64 |          572 |        636 |
| GCC -O0   |          105 |          540 |        645 |
| GCC -O2   |          133 |          335 |        468 |
| Clang -O0 |           90 |          522 |        612 |
| Clang -O2 |          125 |          336 |        461 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    697 us
  lex         bench.c:    137 us
  parse       bench.c:     75 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   2056 us
  peephole    bench.c:    234 us
  link        bench_rcc:  73496 us

RCC -O1:
  preprocess  bench.c:    719 us
  lex         bench.c:    354 us
  parse       bench.c:    212 us
  typecheck   bench.c:     11 us
  opt(CTFE)   bench.c:     32 us
  codegen     bench.c:   2882 us
  peephole    bench.c:    309 us
  link        bench_rcc_o1:  90873 us
```
