# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           15 |          778 |        793 |     3% |
| RCC -O1   |           15 |          768 |        783 |     6% |
| RCC -O2   |           17 |          388 |        405 |    23% |
| RCC -O3   |           17 |          382 |        399 |     3% |
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
| RCC       |          272 |         6455 |       6727 |     1% |
| RCC -O1   |          275 |         7357 |       7632 |     2% |
| RCC -O2   |          327 |         3401 |       3728 |     0% |
| RCC -O3   |          327 |         3331 |       3658 |     0% |
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
  preprocess  bench.c       :   9124 us
  parse       bench.c       :    683 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    312 us
  link        bench_rcc     :    907 us

RCC -O1:
  preprocess  bench.c       :   9036 us
  parse       bench.c       :    703 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     83 us
  codegen     bench.c       :    329 us
  link        bench_o1      :    805 us

RCC -O2:
  preprocess  bench.c       :   9033 us
  parse       bench.c       :    637 us
  typecheck   bench.c       :     17 us
  opt         bench.c       :    100 us
  codegen     bench.c       :   2319 us
  link        bench_o2      :    952 us

RCC -O3:
  preprocess  bench.c       :   8951 us
  parse       bench.c       :    675 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     88 us
  codegen     bench.c       :   2321 us
  link        bench_o3      :    869 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 237587 us
  parse       sqlite3.c     : 173517 us
  typecheck   sqlite3.c     :   8601 us
  codegen     sqlite3.c     : 235138 us
  link        sqlite3.so    :   8936 us

RCC -O1:
  preprocess  sqlite3.c     : 232674 us
  parse       sqlite3.c     : 169748 us
  typecheck   sqlite3.c     :   9002 us
  opt         sqlite3.c     :  72083 us
  codegen     sqlite3.c     : 230543 us
  link        sqlite3.so    :   9154 us

RCC -O2:
  preprocess  sqlite3.c     : 232334 us
  parse       sqlite3.c     : 169778 us
  typecheck   sqlite3.c     :   9215 us
  opt         sqlite3.c     :  83352 us
  codegen     sqlite3.c     : 1704109 us
  link        sqlite3.so    :   9804 us

RCC -O3:
  preprocess  sqlite3.c     : 230654 us
  parse       sqlite3.c     : 165648 us
  typecheck   sqlite3.c     :   8786 us
  opt         sqlite3.c     :  84544 us
  codegen     sqlite3.c     : 1715203 us
  link        sqlite3.so    :   9724 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       754 ms |     0% |
| RCC -O1   |       802 ms |     0% |
| RCC -O2   |      2292 ms |     0% |
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
