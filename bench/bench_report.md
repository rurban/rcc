# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           13 |          668 |        681 |    23% |
| RCC -O1   |           14 |          666 |        680 |    35% |
| RCC -O2   |           13 |          668 |        681 |    46% |
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
| RCC       |          270 |         7772 |       8042 |     7% |
| RCC -O1   |          273 |         8324 |       8597 |     0% |
| RCC -O2   |          273 |         7621 |       7894 |     0% |
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
  preprocess  bench.c       :   7090 us
  parse       bench.c       :    634 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    360 us
  link        bench_rcc     :    679 us

RCC -O1:
  preprocess  bench.c       :   8833 us
  parse       bench.c       :    840 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     95 us
  codegen     bench.c       :    454 us
  link        bench_o1      :    707 us

RCC -O2:
  preprocess  bench.c       :   7045 us
  parse       bench.c       :    672 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     87 us
  codegen     bench.c       :    347 us
  link        bench_o2      :    614 us

RCC -O3:
  preprocess  bench.c       :   8653 us
  parse       bench.c       :    826 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :    137 us
  codegen     bench.c       :    405 us
  link        bench_o3      :    636 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 228954 us
  parse       sqlite3.c     : 174837 us
  typecheck   sqlite3.c     :   9693 us
  codegen     sqlite3.c     : 211044 us
  link        sqlite3.so    :  12651 us

RCC -O1:
  preprocess  sqlite3.c     : 225279 us
  parse       sqlite3.c     : 174367 us
  typecheck   sqlite3.c     :   9538 us
  opt         sqlite3.c     :  62663 us
  codegen     sqlite3.c     : 206711 us
  link        sqlite3.so    :  19416 us

RCC -O2:
  preprocess  sqlite3.c     : 224311 us
  parse       sqlite3.c     : 174158 us
  typecheck   sqlite3.c     :   9713 us
  opt         sqlite3.c     :  73692 us
  codegen     sqlite3.c     : 209754 us
  link        sqlite3.so    :  18881 us

RCC -O3:
  preprocess  sqlite3.c     : 225437 us
  parse       sqlite3.c     : 174611 us
  typecheck   sqlite3.c     :   9569 us
  opt         sqlite3.c     :  89855 us
  codegen     sqlite3.c     : 209168 us
  link        sqlite3.so    :  19095 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       765 ms |     1% |
| RCC -O1   |       794 ms |     0% |
| RCC -O2   |       811 ms |     0% |
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
