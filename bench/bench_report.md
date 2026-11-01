# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           14 |          667 |        681 |     7% |
| RCC -O1   |           13 |          664 |        677 |    30% |
| RCC -O2   |           13 |          664 |        677 |    30% |
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
| RCC       |          273 |         7734 |       8007 |     0% |
| RCC -O1   |          274 |         8309 |       8583 |     1% |
| RCC -O2   |          273 |         7612 |       7885 |     0% |
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
  preprocess  bench.c       :   7063 us
  parse       bench.c       :    641 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    356 us
  link        bench_rcc     :    622 us

RCC -O1:
  preprocess  bench.c       :   8862 us
  parse       bench.c       :    856 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     93 us
  codegen     bench.c       :    469 us
  link        bench_o1      :    764 us

RCC -O2:
  preprocess  bench.c       :   7105 us
  parse       bench.c       :    625 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     86 us
  codegen     bench.c       :    358 us
  link        bench_o2      :    620 us

RCC -O3:
  preprocess  bench.c       :   8628 us
  parse       bench.c       :    818 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :    139 us
  codegen     bench.c       :    421 us
  link        bench_o3      :    626 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 228424 us
  parse       sqlite3.c     : 174015 us
  typecheck   sqlite3.c     :   9736 us
  codegen     sqlite3.c     : 209091 us
  link        sqlite3.so    :  12573 us

RCC -O1:
  preprocess  sqlite3.c     : 225708 us
  parse       sqlite3.c     : 174642 us
  typecheck   sqlite3.c     :   9539 us
  opt         sqlite3.c     :  62622 us
  codegen     sqlite3.c     : 206085 us
  link        sqlite3.so    :  19246 us

RCC -O2:
  preprocess  sqlite3.c     : 225977 us
  parse       sqlite3.c     : 174183 us
  typecheck   sqlite3.c     :   9575 us
  opt         sqlite3.c     :  73964 us
  codegen     sqlite3.c     : 207929 us
  link        sqlite3.so    :  19116 us

RCC -O3:
  preprocess  sqlite3.c     : 225379 us
  parse       sqlite3.c     : 174419 us
  typecheck   sqlite3.c     :   9521 us
  opt         sqlite3.c     :  90072 us
  codegen     sqlite3.c     : 209518 us
  link        sqlite3.so    :  17697 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       766 ms |     0% |
| RCC -O1   |       794 ms |     0% |
| RCC -O2   |       810 ms |     0% |
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
