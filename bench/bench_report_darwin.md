# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           47 |          593 |        640 |
| RCC -O1   |           52 |          585 |        637 |
| RCC -O2   |           50 |          583 |        633 |
| TCC       |           41 |          514 |        555 |
| GCC -O0   |           63 |          434 |        497 |
| GCC -O2   |          110 |          263 |        373 |
| Clang -O0 |           54 |          435 |        489 |
| Clang -O2 |           90 |          295 |        385 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    744 us
  parse       bench.c:    134 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    114 us
  link bench_rcc:    343 us
  link        bench_rcc:  50495 us

RCC -O1:
  preprocess  bench.c:    607 us
  parse       bench.c:    121 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    132 us
  link bench_rcc_o1:    170 us
  link        bench_rcc_o1:  49037 us

RCC -O2:
  preprocess  bench.c:    594 us
  parse       bench.c:    138 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    134 us
  link bench_rcc_o2:    165 us
  link        bench_rcc_o2:  51746 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 233518 us
  parse       sqlite3.c:  47771 us
  typecheck   sqlite3.c:  13496 us
  codegen     sqlite3.c:  80554 us
  link libsqlite3.so:  13438 us

RCC -O1:
  preprocess  sqlite3.c: 178483 us
  parse       sqlite3.c:  40153 us
  typecheck   sqlite3.c:  11195 us
  opt         sqlite3.c:  18143 us
  codegen     sqlite3.c:  81215 us
  link libsqlite3.so:  15031 us

RCC -O2:
  preprocess  sqlite3.c: 181651 us
  parse       sqlite3.c:  41007 us
  typecheck   sqlite3.c:  11115 us
  opt         sqlite3.c: 121042 us
  codegen     sqlite3.c:  81443 us
  link libsqlite3.so:  13841 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       743 ms |
| RCC -O1   |       702 ms |
| RCC -O2   |       782 ms |
| TCC       |       115 ms |
| GCC -O0   |      1157 ms |
| GCC -O2   |     11488 ms |
| Clang -O0 |       946 ms |
| Clang -O2 |      8916 ms |
