# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           87 |          650 |        737 |
| RCC -O1   |          131 |          681 |        812 |
| TCC       |           93 |          688 |        781 |
| GCC -O0   |          168 |          583 |        751 |
| GCC -O2   |          167 |          324 |        491 |
| Clang -O0 |           78 |          577 |        655 |
| Clang -O2 |          158 |          355 |        513 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    917 us
  lex         bench.c:    104 us
  parse       bench.c:     66 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   2440 us
  peephole    bench.c:    322 us
  link        bench_rcc: 125175 us

RCC -O1:
  preprocess  bench.c:    559 us
  lex         bench.c:    157 us
  parse       bench.c:     55 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1889 us
  peephole    bench.c:    356 us
  link        bench_rcc_o1:  81581 us
```
