# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           99 |          647 |        746 |
| RCC -O1   |          102 |          643 |        745 |
| TCC       |           50 |          568 |        618 |
| GCC -O0   |           74 |          469 |        543 |
| GCC -O2   |          116 |          285 |        401 |
| Clang -O0 |           81 |          496 |        577 |
| Clang -O2 |           86 |          303 |        389 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    484 us
  lex         bench.c:     99 us
  parse       bench.c:     50 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1240 us
  peephole    bench.c:    231 us
  link        bench_rcc:  59436 us

RCC -O1:
  preprocess  bench.c:    397 us
  lex         bench.c:     98 us
  parse       bench.c:     50 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:   1305 us
  peephole    bench.c:    229 us
  link        bench_rcc_o1:  64384 us
```
