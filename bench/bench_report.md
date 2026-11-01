# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           13 |          668 |        681 |    23% |
| RCC -O1   |           13 |          666 |        679 |    30% |
| RCC -O2   |           13 |          668 |        681 |    30% |
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
| RCC       |          271 |         7775 |       8046 |     4% |
| RCC -O1   |          271 |         8332 |       8603 |     4% |
| RCC -O2   |          271 |         7633 |       7904 |     4% |
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
  preprocess  bench.c       :   7116 us
  parse       bench.c       :    636 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    360 us
  link        bench_rcc     :    630 us

RCC -O1:
  preprocess  bench.c       :   9299 us
  parse       bench.c       :    845 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     91 us
  codegen     bench.c       :    439 us
  link        bench_o1      :   1178 us

RCC -O2:
  preprocess  bench.c       :   7135 us
  parse       bench.c       :    630 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     87 us
  codegen     bench.c       :    357 us
  link        bench_o2      :    620 us

RCC -O3:
  preprocess  bench.c       :   9758 us
  parse       bench.c       :    644 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    120 us
  codegen     bench.c       :    343 us
  link        bench_o3      :    630 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 235393 us
  parse       sqlite3.c     : 177082 us
  typecheck   sqlite3.c     :   9542 us
  codegen     sqlite3.c     : 209637 us
  link        sqlite3.so    :  13924 us

RCC -O1:
  preprocess  sqlite3.c     : 226792 us
  parse       sqlite3.c     : 177332 us
  typecheck   sqlite3.c     :   9584 us
  opt         sqlite3.c     :  62719 us
  codegen     sqlite3.c     : 206171 us
  link        sqlite3.so    :  19176 us

RCC -O2:
  preprocess  sqlite3.c     : 225263 us
  parse       sqlite3.c     : 178177 us
  typecheck   sqlite3.c     :   9533 us
  opt         sqlite3.c     :  73945 us
  codegen     sqlite3.c     : 207900 us
  link        sqlite3.so    :  20138 us

RCC -O3:
  preprocess  sqlite3.c     : 226811 us
  parse       sqlite3.c     : 177401 us
  typecheck   sqlite3.c     :   9545 us
  opt         sqlite3.c     :  90069 us
  codegen     sqlite3.c     : 208608 us
  link        sqlite3.so    :  20185 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       768 ms |     1% |
| RCC -O1   |       796 ms |     0% |
| RCC -O2   |       812 ms |     0% |
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
