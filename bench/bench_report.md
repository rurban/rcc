# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           17 |          778 |        795 |    11% |
| RCC -O1   |           19 |          779 |        798 |    10% |
| RCC -O2   |           26 |          454 |        480 |    26% |
| RCC -O3   |           20 |          454 |        474 |    70% |
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
| RCC       |          366 |         9440 |       9806 |     9% |
| RCC -O1   |          366 |         9995 |      10361 |    10% |
| RCC -O2   |          358 |         5000 |       5358 |    33% |
| RCC -O3   |          366 |         5025 |       5391 |    10% |
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
  preprocess  bench.c       :   9200 us
  parse       bench.c       :    875 us
  typecheck   bench.c       :      7 us
  codegen     bench.c       :    486 us
  link        bench_rcc     :    922 us

RCC -O1:
  preprocess  bench.c       :  10657 us
  parse       bench.c       :   1697 us
  typecheck   bench.c       :     61 us
  opt         bench.c       :    185 us
  codegen     bench.c       :    575 us
  link        bench_o1      :   1015 us

RCC -O2:
  preprocess  bench.c       :   8580 us
  parse       bench.c       :    716 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     97 us
  codegen     bench.c       :   2246 us
  link        bench_o2      :    995 us

RCC -O3:
  preprocess  bench.c       :   9484 us
  parse       bench.c       :   1942 us
  typecheck   bench.c       :     60 us
  opt         bench.c       :    261 us
  codegen     bench.c       :   3203 us
  link        bench_o3      :   1039 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 276998 us
  parse       sqlite3.c     : 207629 us
  typecheck   sqlite3.c     :  15454 us
  codegen     sqlite3.c     : 230995 us
  link        sqlite3.so    :  11362 us

RCC -O1:
  preprocess  sqlite3.c     : 253496 us
  parse       sqlite3.c     : 214587 us
  typecheck   sqlite3.c     :  12541 us
  opt         sqlite3.c     :  77714 us
  codegen     sqlite3.c     : 239635 us
  link        sqlite3.so    :  11179 us

RCC -O2:
  preprocess  sqlite3.c     : 246445 us
  parse       sqlite3.c     : 208445 us
  typecheck   sqlite3.c     :  16278 us
  opt         sqlite3.c     :  93741 us
  codegen     sqlite3.c     : 1041404 us
  link        sqlite3.so    :  10369 us

RCC -O3:
  preprocess  sqlite3.c     : 258081 us
  parse       sqlite3.c     : 186540 us
  typecheck   sqlite3.c     :  11260 us
  opt         sqlite3.c     : 105001 us
  codegen     sqlite3.c     : 1033593 us
  link        sqlite3.so    :  10300 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       845 ms |     2% |
| RCC -O1   |       887 ms |     0% |
| RCC -O2   |      1720 ms |     2% |
| RCC -O3   |      1730 ms |     1% |
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
