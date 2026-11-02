# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           16 |          779 |        795 |     6% |
| RCC -O1   |           16 |          770 |        786 |     6% |
| RCC -O2   |           17 |          788 |        805 |     3% |
| RCC -O3   |           17 |          760 |        777 |     4% |
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
| RCC       |          303 |         6551 |       6854 |     0% |
| RCC -O1   |          304 |         7359 |       7663 |     0% |
| RCC -O2   |          304 |         6544 |       6848 |     2% |
| RCC -O3   |          309 |         6524 |       6833 |     0% |
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
  preprocess  bench.c       :  11047 us
  parse       bench.c       :    660 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    330 us
  link        bench_rcc     :    892 us

RCC -O1:
  preprocess  bench.c       :  10591 us
  parse       bench.c       :    593 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     77 us
  codegen     bench.c       :    337 us
  link        bench_o1      :    798 us

RCC -O2:
  preprocess  bench.c       :  10535 us
  parse       bench.c       :    613 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     89 us
  codegen     bench.c       :    364 us
  link        bench_o2      :    848 us

RCC -O3:
  preprocess  bench.c       :  10364 us
  parse       bench.c       :    611 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    119 us
  codegen     bench.c       :    394 us
  link        bench_o3      :    863 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 271379 us
  parse       sqlite3.c     : 166621 us
  typecheck   sqlite3.c     :   9182 us
  codegen     sqlite3.c     : 224577 us
  link        sqlite3.so    :   8654 us

RCC -O1:
  preprocess  sqlite3.c     : 269066 us
  parse       sqlite3.c     : 162788 us
  typecheck   sqlite3.c     :   8842 us
  opt         sqlite3.c     :  70944 us
  codegen     sqlite3.c     : 223503 us
  link        sqlite3.so    :  16437 us

RCC -O2:
  preprocess  sqlite3.c     : 269713 us
  parse       sqlite3.c     : 161455 us
  typecheck   sqlite3.c     :   8813 us
  opt         sqlite3.c     :  82293 us
  codegen     sqlite3.c     : 252152 us
  link        sqlite3.so    :  22709 us

RCC -O3:
  preprocess  sqlite3.c     : 270696 us
  parse       sqlite3.c     : 161015 us
  typecheck   sqlite3.c     :   8664 us
  opt         sqlite3.c     :  99587 us
  codegen     sqlite3.c     : 251975 us
  link        sqlite3.so    :  17025 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       786 ms |     1% |
| RCC -O1   |       842 ms |     0% |
| RCC -O2   |       882 ms |     1% |
| RCC -O3   |       889 ms |     2% |
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
