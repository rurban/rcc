# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           56 |          630 |        686 |
| RCC -O1   |           59 |          630 |        689 |
| RCC -O2   |           54 |          627 |        681 |
| TCC       |           48 |          557 |        605 |
| GCC -O0   |           71 |          470 |        541 |
| GCC -O2   |          100 |          284 |        384 |
| Clang -O0 |           58 |          468 |        526 |
| Clang -O2 |           89 |          283 |        372 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    771 us
  parse       bench.c:    155 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    136 us
  link bench_rcc:    133 us
  link        bench_rcc:  68788 us

RCC -O1:
  preprocess  bench.c:    603 us
  parse       bench.c:    160 us
  typecheck   bench.c:      6 us
  opt         bench.c:     21 us
  codegen     bench.c:    186 us
  link bench_rcc_o1:    142 us
  link        bench_rcc_o1:  59718 us

RCC -O2:
  preprocess  bench.c:    864 us
  parse       bench.c:    151 us
  typecheck   bench.c:      4 us
  opt         bench.c:     18 us
  codegen     bench.c:    281 us
  link bench_rcc_o2:    124 us
  link        bench_rcc_o2:  54646 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 295264 us
  parse       sqlite3.c:  62266 us
  typecheck   sqlite3.c:  13728 us
  codegen     sqlite3.c: 113621 us
  link libsqlite3.so:  16360 us

RCC -O1:
  preprocess  sqlite3.c: 262185 us
  parse       sqlite3.c:  55741 us
  typecheck   sqlite3.c:  15542 us
  opt         sqlite3.c:  21789 us
  codegen     sqlite3.c:  93431 us
  link libsqlite3.so:  15309 us

RCC -O2:
  preprocess  sqlite3.c: 206572 us
  parse       sqlite3.c:  45958 us
  typecheck   sqlite3.c:  12935 us
  opt         sqlite3.c: 130970 us
  codegen     sqlite3.c:  88760 us
  link libsqlite3.so:  15895 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       626 ms |
| RCC -O1   |       681 ms |
| RCC -O2   |       706 ms |
| TCC       |        93 ms |
| GCC -O0   |      1017 ms |
| GCC -O2   |     10116 ms |
| Clang -O0 |      1310 ms |
| Clang -O2 |     13925 ms |
