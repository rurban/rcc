# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           24 |          700 |        724 |     5% |
| RCC -O1   |           24 |          699 |        723 |     8% |
| RCC -O2   |           23 |          696 |        719 |     4% |
| TCC       |            5 |          491 |        496 |    20% |
| SLIMCC    |           32 |          501 |        533 |     9% |
| XCC       |            8 |          359 |        367 |    12% |
| KEFIR     |          177 |          572 |        749 |     2% |
| KEFIR -O1 |          188 |          308 |        496 |     1% |
| SCC       |           32 |          539 |        571 |    16% |
| LACC      |           24 |          759 |        783 |     4% |
| ANTCC     |           24 |          415 |        439 |     4% |
| CAKE      |           88 |          480 |        568 |     5% |
| CCC       |           32 |          540 |        572 |    14% |
| GCC -O0   |           54 |          482 |        536 |     7% |
| GCC -O2   |          145 |          176 |        321 |     3% |
| Clang -O0 |           77 |          466 |        543 |     3% |
| Clang -O2 |          126 |          176 |        302 |     2% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          369 |         5889 |       6258 |     5% |
| RCC -O1   |          377 |         6694 |       7071 |     0% |
| RCC -O2   |          374 |         6156 |       6530 |     2% |
| TCC       |           66 |         5115 |       5181 |     6% |
| SLIMCC    |          252 |         4944 |       5196 |     1% |
| KEFIR     |         4451 |         5314 |       9765 |     0% |
| KEFIR -O1 |         4898 |         3817 |       8715 |    74% |
| ANTCC     |          220 |         3870 |       4090 |    16% |
| CCC       |          872 |         5033 |       5905 |    40% |
| GCC -O0   |          906 |         5430 |       6336 |    22% |
| GCC -O2   |         1856 |         2544 |       4400 |     0% |
| Clang -O0 |          868 |         4632 |       5500 |     0% |
| Clang -O2 |         1603 |         2580 |       4183 |     6% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  11601 us
  parse       bench.c       :    617 us
  typecheck   bench.c       :     11 us
  codegen     bench.c       :    373 us
  link        bench_rcc     :   9716 us

RCC -O1:
  preprocess  bench.c       :  11101 us
  parse       bench.c       :    858 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     45 us
  codegen     bench.c       :    301 us
  link        bench_o1      :   9534 us

RCC -O2:
  preprocess  bench.c       :  10149 us
  parse       bench.c       :    637 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     63 us
  codegen     bench.c       :    365 us
  link        bench_o2      :   9858 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 251526 us
  parse       sqlite3.c     : 149951 us
  typecheck   sqlite3.c     :   9245 us
  codegen     sqlite3.c     : 204030 us
  link        sqlite3.so    :  11335 us

RCC -O1:
  preprocess  sqlite3.c     : 247058 us
  parse       sqlite3.c     : 148764 us
  typecheck   sqlite3.c     :   8630 us
  opt         sqlite3.c     :  36471 us
  codegen     sqlite3.c     : 200941 us
  link        sqlite3.so    :  11430 us

RCC -O2:
  preprocess  sqlite3.c     : 250484 us
  parse       sqlite3.c     : 148012 us
  typecheck   sqlite3.c     :   8391 us
  opt         sqlite3.c     :  46008 us
  codegen     sqlite3.c     : 202350 us
  link        sqlite3.so    :  10922 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       731 ms |     0% |
| RCC -O1   |       734 ms |     1% |
| RCC -O2   |       749 ms |     2% |
| TCC       |       101 ms |     1% |
| SLIMCC    |       661 ms |     1% |
| KEFIR     |     18543 ms |     0% |
| KEFIR -O1 |     33734 ms |     0% |
| ANTCC     |       400 ms |     0% |
| CCC       |     12750 ms |     0% |
| GCC -O0   |      4052 ms |     0% |
| GCC -O2   |     25792 ms |     1% |
| Clang -O0 |      1804 ms |     0% |
| Clang -O2 |     20082 ms |     0% |
