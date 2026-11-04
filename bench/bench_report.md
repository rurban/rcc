# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           17 |          771 |        788 |     5% |
| RCC -O1   |           17 |          783 |        800 |     3% |
| RCC -O2   |           19 |          385 |        404 |     1% |
| RCC -O3   |           18 |          385 |        403 |     5% |
| TCC       |            5 |          548 |        553 |    20% |
| SLIMCC    |           34 |          557 |        591 |     2% |
| XCC       |            8 |          398 |        406 |    12% |
| KEFIR     |          190 |          640 |        830 |     1% |
| KEFIR -O1 |          198 |          343 |        541 |     1% |
| SCC       |           36 |          609 |        645 |    10% |
| LACC      |           23 |          831 |        854 |     4% |
| ANTCC     |           24 |          460 |        484 |     4% |
| CAKE      |           92 |          533 |        625 |     2% |
| BCC       |           29 |          621 |        650 |     0% |
| CCC       |           34 |          598 |        632 |     0% |
| GCC -O0   |           57 |          535 |        592 |     1% |
| GCC -O2   |          160 |          196 |        356 |     0% |
| Clang -O0 |           83 |          516 |        599 |     1% |
| Clang -O2 |          138 |          199 |        337 |     0% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          297 |         6536 |       6833 |     2% |
| RCC -O1   |          298 |         7247 |       7545 |     1% |
| RCC -O2   |          348 |         3335 |       3683 |     1% |
| RCC -O3   |          356 |         3345 |       3701 |     0% |
| TCC       |           71 |         5837 |       5908 |     2% |
| SLIMCC    |          290 |         5681 |       5971 |     7% |
| KEFIR     |         4992 |         6130 |      11122 |    71% |
| KEFIR -O1 |         5508 |         4269 |       9777 |     1% |
| ANTCC     |          202 |         4992 |       5194 |    39% |
| CCC       |         1034 |         5774 |       6808 |    12% |
| BCC       |         3611 |         5782 |       9393 |     4% |
| GCC -O0   |          972 |         5802 |       6774 |    12% |
| GCC -O2   |         2155 |         3310 |       5465 |    21% |
| Clang -O0 |         1394 |         5545 |       6939 |    57% |
| Clang -O2 |         1763 |         3202 |       4965 |    10% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  10340 us
  parse       bench.c       :    606 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    364 us
  link        bench_rcc     :    866 us

RCC -O1:
  preprocess  bench.c       :  10183 us
  parse       bench.c       :    588 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     81 us
  codegen     bench.c       :    370 us
  link        bench_o1      :    867 us

RCC -O2:
  preprocess  bench.c       :  10226 us
  parse       bench.c       :    618 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     90 us
  codegen     bench.c       :   2225 us
  link        bench_o2      :    905 us

RCC -O3:
  preprocess  bench.c       :  10322 us
  parse       bench.c       :    563 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    121 us
  codegen     bench.c       :   2224 us
  link        bench_o3      :    849 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 269574 us
  parse       sqlite3.c     : 162288 us
  typecheck   sqlite3.c     :   8523 us
  codegen     sqlite3.c     : 225609 us
  link        sqlite3.so    :   8752 us

RCC -O1:
  preprocess  sqlite3.c     : 267755 us
  parse       sqlite3.c     : 161813 us
  typecheck   sqlite3.c     :   8572 us
  opt         sqlite3.c     :  69661 us
  codegen     sqlite3.c     : 225968 us
  link        sqlite3.so    :   9170 us

RCC -O2:
  preprocess  sqlite3.c     : 267113 us
  parse       sqlite3.c     : 160230 us
  typecheck   sqlite3.c     :   8552 us
  opt         sqlite3.c     :  80900 us
  codegen     sqlite3.c     : 1678675 us
  link        sqlite3.so    :   9473 us

RCC -O3:
  preprocess  sqlite3.c     : 268047 us
  parse       sqlite3.c     : 160891 us
  typecheck   sqlite3.c     :   8657 us
  opt         sqlite3.c     :  98453 us
  codegen     sqlite3.c     : 1684405 us
  link        sqlite3.so    :   9767 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       779 ms |     0% |
| RCC -O1   |       822 ms |     0% |
| RCC -O2   |      2275 ms |     1% |
| RCC -O3   |      2315 ms |     0% |
| TCC       |       116 ms |     0% |
| SLIMCC    |       776 ms |     0% |
| KEFIR     |     20966 ms |     0% |
| KEFIR -O1 |     39080 ms |     2% |
| ANTCC     |       454 ms |     0% |
| CCC       |     14753 ms |     6% |
| GCC -O0   |      4869 ms |     0% |
| GCC -O2   |     30896 ms |     0% |
| Clang -O0 |      2018 ms |     5% |
| Clang -O2 |     25578 ms |    35% |
