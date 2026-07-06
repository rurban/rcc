# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           72 |          717 |        789 |
| RCC -O1   |          133 |          660 |        793 |
| TCC       |           81 |          559 |        640 |
| GCC -O0   |           91 |          478 |        569 |
| GCC -O2   |          120 |          297 |        417 |
| Clang -O0 |           89 |          491 |        580 |
| Clang -O2 |          108 |          312 |        420 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1345 us
  lex         bench.c:     74 us
  parse       bench.c:    135 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    141 us
  link        bench_rcc:  73918 us

RCC -O1:
  preprocess  bench.c:    466 us
  lex         bench.c:     72 us
  parse       bench.c:    111 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     31 us
  codegen     bench.c:    150 us
  link        bench_rcc_o1:  68324 us
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
| TCC       |       164 ms |
| GCC -O0   |      1189 ms |
| GCC -O2   |     15991 ms |
| Clang -O0 |      1978 ms |
| Clang -O2 |     11733 ms |
