# Darwin RCC Benchmark Results

_Generated: May 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           74 |          624 |        698 |
| RCC -O1   |           80 |          624 |        704 |
| TCC       |           44 |          556 |        600 |
| GCC -O0   |           82 |          462 |        544 |
| GCC -O2   |          103 |          279 |        382 |
| Clang -O0 |           58 |          471 |        529 |
| Clang -O2 |          108 |          287 |        395 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    507 us
  lex         bench.c:     74 us
  parse       bench.c:     60 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   2119 us
  peephole    bench.c:    242 us
  link        bench_rcc:  73989 us

RCC -O1:
  preprocess  bench.c:    476 us
  lex         bench.c:     68 us
  parse       bench.c:     53 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1662 us
  peephole    bench.c:    237 us
  link        bench_rcc_o1:  65109 us
```
