# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           89 |          653 |        742 |
| RCC -O1   |           88 |          652 |        740 |
| RCC -O2   |           56 |          649 |        705 |
| TCC       |           38 |          573 |        611 |
| GCC -O0   |           68 |          480 |        548 |
| GCC -O2   |           93 |          287 |        380 |
| Clang -O0 |           66 |          488 |        554 |
| Clang -O2 |           97 |          286 |        383 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    613 us
  parse       bench.c:    114 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    106 us
  link        bench_rcc:  61003 us

RCC -O1:
  preprocess  bench.c:    580 us
  parse       bench.c:    118 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    106 us
  link        bench_rcc_o1:  51994 us

RCC -O2:
  preprocess  bench.c:    535 us
  parse       bench.c:    110 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    105 us
  link        bench_rcc_o2:  50438 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 359965 us
  parse       sqlite3.c:  52395 us
  typecheck   sqlite3.c:  22894 us
  codegen     sqlite3.c:  58839 us

RCC -O1:
  preprocess  sqlite3.c: 245130 us
  parse       sqlite3.c:  47331 us
  typecheck   sqlite3.c:  14687 us
  opt         sqlite3.c:  21416 us
  codegen     sqlite3.c:  51462 us

RCC -O2:
  preprocess  sqlite3.c: 456861 us
  parse       sqlite3.c:  48509 us
  typecheck   sqlite3.c:  19302 us
  opt         sqlite3.c: 161552 us
  codegen     sqlite3.c: 101528 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       556 ms |
| RCC -O1   |       527 ms |
| RCC -O2   |       637 ms |
| TCC       |        68 ms |
| GCC -O0   |      1032 ms |
| GCC -O2   |     10464 ms |
| Clang -O0 |      1026 ms |
| Clang -O2 |      9814 ms |
