# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           97 |          734 |        831 |
| RCC -O1   |          112 |          698 |        810 |
| TCC       |          110 |          579 |        689 |
| GCC -O0   |          167 |          480 |        647 |
| GCC -O2   |          145 |          311 |        456 |
| Clang -O0 |           72 |          480 |        552 |
| Clang -O2 |          158 |          322 |        480 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    935 us
  lex         bench.c:    252 us
  parse       bench.c:    127 us
  typecheck   bench.c:      9 us
  codegen     bench.c:   3939 us
  peephole    bench.c:    442 us
  link        bench_rcc:  79470 us

RCC -O1:
  preprocess  bench.c:    419 us
  lex         bench.c:    103 us
  parse       bench.c:    129 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   2410 us
  peephole    bench.c:    312 us
  link        bench_rcc_o1:  73362 us
```
