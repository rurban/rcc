# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           15 |          781 |        796 |     6% |
| RCC -O1   |           15 |          780 |        795 |     4% |
| RCC -O2   |           17 |          390 |        407 |     5% |
| RCC -O3   |           17 |          387 |        404 |     0% |
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
| RCC       |          276 |         6503 |       6779 |    13% |
| RCC -O1   |          279 |         7355 |       7634 |     0% |
| RCC -O2   |          325 |         3418 |       3743 |     1% |
| RCC -O3   |          327 |         3412 |       3739 |     0% |
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
  preprocess  bench.c       :   8894 us
  parse       bench.c       :    685 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    405 us
  link        bench_rcc     :    948 us

RCC -O1:
  preprocess  bench.c       :   8976 us
  parse       bench.c       :    644 us
  typecheck   bench.c       :     17 us
  opt         bench.c       :     83 us
  codegen     bench.c       :    329 us
  link        bench_o1      :    866 us

RCC -O2:
  preprocess  bench.c       :   9259 us
  parse       bench.c       :    697 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     89 us
  codegen     bench.c       :   2428 us
  link        bench_o2      :   1046 us

RCC -O3:
  preprocess  bench.c       :   9321 us
  parse       bench.c       :    678 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     89 us
  codegen     bench.c       :   2301 us
  link        bench_o3      :    890 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 248939 us
  parse       sqlite3.c     : 176106 us
  typecheck   sqlite3.c     :   9165 us
  codegen     sqlite3.c     : 235545 us
  link        sqlite3.so    :   8933 us

RCC -O1:
  preprocess  sqlite3.c     : 233728 us
  parse       sqlite3.c     : 172772 us
  typecheck   sqlite3.c     :   9180 us
  opt         sqlite3.c     :  74652 us
  codegen     sqlite3.c     : 233799 us
  link        sqlite3.so    :   9078 us

RCC -O2:
  preprocess  sqlite3.c     : 230605 us
  parse       sqlite3.c     : 165780 us
  typecheck   sqlite3.c     :   8911 us
  opt         sqlite3.c     :  83098 us
  codegen     sqlite3.c     : 1666962 us
  link        sqlite3.so    :   9653 us

RCC -O3:
  preprocess  sqlite3.c     : 232549 us
  parse       sqlite3.c     : 168259 us
  typecheck   sqlite3.c     :   8993 us
  opt         sqlite3.c     :  84039 us
  codegen     sqlite3.c     : 1705567 us
  link        sqlite3.so    :   9719 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       770 ms |     0% |
| RCC -O1   |       792 ms |     0% |
| RCC -O2   |      2273 ms |     1% |
| RCC -O3   |      2282 ms |     0% |
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
