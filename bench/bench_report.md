# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           14 |          701 |        715 |     4% |
| RCC -O1   |           14 |          700 |        714 |     7% |
| RCC -O2   |           14 |          701 |        715 |     7% |
| TCC       |            5 |          487 |        492 |     0% |
| SLIMCC    |           31 |          500 |        531 |     3% |
| XCC       |            8 |          357 |        365 |     1% |
| KEFIR     |          178 |          572 |        750 |     1% |
| KEFIR -O1 |          186 |          307 |        493 |     1% |
| SCC       |           32 |          537 |        569 |    16% |
| LACC      |           21 |          754 |        775 |     4% |
| ANTCC     |           23 |          414 |        437 |     0% |
| CAKE      |           89 |          479 |        568 |     1% |
| BCC       |           27 |          558 |        585 |     3% |
| CCC       |           33 |          535 |        568 |    16% |
| GCC -O0   |           53 |          479 |        532 |     1% |
| GCC -O2   |          144 |          175 |        319 |     0% |
| Clang -O0 |           78 |          462 |        540 |     1% |
| Clang -O2 |          127 |          177 |        304 |     2% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          274 |         5805 |       6079 |     0% |
| RCC -O1   |          274 |         6540 |       6814 |     1% |
| RCC -O2   |          274 |         6063 |       6337 |     1% |
| TCC       |           65 |         5032 |       5097 |     1% |
| SLIMCC    |          248 |         4914 |       5162 |     1% |
| KEFIR     |         4383 |         5280 |       9663 |     0% |
| KEFIR -O1 |         4840 |         3688 |       8528 |     0% |
| ANTCC     |          186 |         3707 |       3893 |     1% |
| CCC       |          859 |         4470 |       5329 |     0% |
| BCC       |         2972 |         4668 |       7640 |     3% |
| GCC -O0   |          834 |         4806 |       5640 |     1% |
| GCC -O2   |         1844 |         2534 |       4378 |     0% |
| Clang -O0 |          861 |         4577 |       5438 |     0% |
| Clang -O2 |         1577 |         2550 |       4127 |     0% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   9849 us
  parse       bench.c       :    594 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    400 us
  link        bench_rcc     :    869 us

RCC -O1:
  preprocess  bench.c       :  13572 us
  parse       bench.c       :    573 us
  typecheck   bench.c       :      9 us
  opt         bench.c       :    146 us
  codegen     bench.c       :    317 us
  link        bench_o1      :    891 us

RCC -O2:
  preprocess  bench.c       :  10341 us
  parse       bench.c       :    643 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     83 us
  codegen     bench.c       :    321 us
  link        bench_o2      :    825 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 252987 us
  parse       sqlite3.c     : 151842 us
  typecheck   sqlite3.c     :   8189 us
  codegen     sqlite3.c     : 207664 us
  link        sqlite3.so    :   8800 us

RCC -O1:
  preprocess  sqlite3.c     : 253227 us
  parse       sqlite3.c     : 149533 us
  typecheck   sqlite3.c     :   8262 us
  opt         sqlite3.c     :  64478 us
  codegen     sqlite3.c     : 206339 us
  link        sqlite3.so    :   9227 us

RCC -O2:
  preprocess  sqlite3.c     : 249053 us
  parse       sqlite3.c     : 149344 us
  typecheck   sqlite3.c     :   8415 us
  opt         sqlite3.c     :  74075 us
  codegen     sqlite3.c     : 208906 us
  link        sqlite3.so    :   9103 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       735 ms |     0% |
| RCC -O1   |       760 ms |     0% |
| RCC -O2   |       775 ms |     0% |
| TCC       |        97 ms |     0% |
| SLIMCC    |       645 ms |     2% |
| KEFIR     |     18304 ms |     0% |
| KEFIR -O1 |     33028 ms |     0% |
| ANTCC     |       387 ms |     1% |
| CCC       |     12643 ms |     0% |
| GCC -O0   |      4019 ms |     0% |
| GCC -O2   |     25547 ms |     0% |
| Clang -O0 |      1783 ms |     0% |
| Clang -O2 |     19890 ms |     0% |
