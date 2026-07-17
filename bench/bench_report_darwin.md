# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           51 |          607 |        658 |
| RCC -O1   |           50 |          657 |        707 |
| TCC       |           38 |          566 |        604 |
| GCC -O0   |           78 |          473 |        551 |
| GCC -O2   |          105 |          277 |        382 |
| Clang -O0 |           68 |          467 |        535 |
| Clang -O2 |           91 |          271 |        362 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    591 us
  lex         bench.c:     72 us
  parse       bench.c:    102 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    119 us
  link        bench_rcc:  62668 us

RCC -O1:
  preprocess  bench.c:    469 us
  lex         bench.c:     74 us
  parse       bench.c:    100 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    121 us
  link        bench_rcc_o1:  48272 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       109 ms |
| GCC -O0   |       967 ms |
| GCC -O2   |      9403 ms |
| Clang -O0 |       995 ms |
| Clang -O2 |      9456 ms |
