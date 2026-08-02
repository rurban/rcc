# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           79 |          592 |        671 |
| RCC -O1   |           49 |          592 |        641 |
| RCC -O2   |           49 |          640 |        689 |
| TCC       |           61 |          515 |        576 |
| GCC -O0   |           60 |          436 |        496 |
| GCC -O2   |           92 |          262 |        354 |
| Clang -O0 |           50 |          441 |        491 |
| Clang -O2 |           85 |          273 |        358 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    746 us
  parse       bench.c:    152 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    125 us
  link bench_rcc:    103 us
  link        bench_rcc:  50303 us

RCC -O1:
  preprocess  bench.c:    792 us
  parse       bench.c:    117 us
  typecheck   bench.c:      5 us
  opt         bench.c:     18 us
  codegen     bench.c:    112 us
  link bench_rcc_o1:     84 us
  link        bench_rcc_o1:  47121 us

RCC -O2:
  preprocess  bench.c:    560 us
  parse       bench.c:    112 us
  typecheck   bench.c:      4 us
  opt         bench.c:     19 us
  codegen     bench.c:    115 us
  link bench_rcc_o2:    195 us
  link        bench_rcc_o2:  43303 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 187141 us
  parse       sqlite3.c:  83535 us
  typecheck   sqlite3.c:  16037 us
  codegen     sqlite3.c:  92115 us
  link libsqlite3.so:  20500 us

RCC -O1:
  preprocess  sqlite3.c: 225565 us
  parse       sqlite3.c:  58267 us
  typecheck   sqlite3.c:  15355 us
  opt         sqlite3.c:  19632 us
  codegen     sqlite3.c:  83094 us
  link libsqlite3.so:  14862 us

RCC -O2:
  preprocess  sqlite3.c: 244506 us
  parse       sqlite3.c:  42298 us
  typecheck   sqlite3.c:  11435 us
  opt         sqlite3.c: 124396 us
  codegen     sqlite3.c:  83809 us
  link libsqlite3.so:  17351 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       769 ms |
| RCC -O1   |       626 ms |
| RCC -O2   |       644 ms |
| TCC       |       102 ms |
| GCC -O0   |      1062 ms |
| GCC -O2   |      9428 ms |
| Clang -O0 |       950 ms |
| Clang -O2 |      8924 ms |
