# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           17 |          780 |        797 |     4% |
| RCC -O1   |           17 |          768 |        785 |     6% |
| RCC -O2   |           19 |          388 |        407 |     1% |
| RCC -O3   |           19 |          384 |        403 |     1% |
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
| RCC       |          296 |         6448 |       6744 |     2% |
| RCC -O1   |          300 |         7197 |       7497 |     2% |
| RCC -O2   |          344 |         3413 |       3757 |     0% |
| RCC -O3   |          348 |         3430 |       3778 |     0% |
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
  preprocess  bench.c       :  10401 us
  parse       bench.c       :    575 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    408 us
  link        bench_rcc     :    891 us

RCC -O1:
  preprocess  bench.c       :  10233 us
  parse       bench.c       :    575 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    129 us
  codegen     bench.c       :    345 us
  link        bench_o1      :    860 us

RCC -O2:
  preprocess  bench.c       :  10623 us
  parse       bench.c       :    628 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     86 us
  codegen     bench.c       :   2073 us
  link        bench_o2      :    911 us

RCC -O3:
  preprocess  bench.c       :  10330 us
  parse       bench.c       :    627 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    111 us
  codegen     bench.c       :   1982 us
  link        bench_o3      :    909 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 277511 us
  parse       sqlite3.c     : 168516 us
  typecheck   sqlite3.c     :   9036 us
  codegen     sqlite3.c     : 229686 us
  link        sqlite3.so    :   9596 us

RCC -O1:
  preprocess  sqlite3.c     : 271276 us
  parse       sqlite3.c     : 166636 us
  typecheck   sqlite3.c     :   8934 us
  opt         sqlite3.c     :  71155 us
  codegen     sqlite3.c     : 228020 us
  link        sqlite3.so    :   9391 us

RCC -O2:
  preprocess  sqlite3.c     : 274347 us
  parse       sqlite3.c     : 164918 us
  typecheck   sqlite3.c     :   9191 us
  opt         sqlite3.c     :  83621 us
  codegen     sqlite3.c     : 1121322 us
  link        sqlite3.so    :   8919 us

RCC -O3:
  preprocess  sqlite3.c     : 270554 us
  parse       sqlite3.c     : 162756 us
  typecheck   sqlite3.c     :   8619 us
  opt         sqlite3.c     :  98640 us
  codegen     sqlite3.c     : 1114150 us
  link        sqlite3.so    :   8974 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       799 ms |     1% |
| RCC -O1   |       843 ms |     0% |
| RCC -O2   |      1753 ms |     1% |
| RCC -O3   |      1730 ms |     0% |
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
