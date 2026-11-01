# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           17 |          832 |        849 |      - |
| RCC -O1   |           17 |          807 |        824 |      - |
| RCC -O2   |           18 |          824 |        842 |      - |
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
| RCC       |           17 |          832 |        849 |      - |
| RCC -O1   |           17 |          807 |        824 |      - |
| RCC -O2   |           18 |          824 |        842 |      - |
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
  preprocess  bench.c       :  11108 us
  parse       bench.c       :    644 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    386 us
  link        bench_rcc     :    928 us

RCC -O1:
  preprocess  bench.c       :  10870 us
  parse       bench.c       :    696 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     90 us
  codegen     bench.c       :    387 us
  link        bench_o1      :    969 us

RCC -O2:
  preprocess  bench.c       :  11730 us
  parse       bench.c       :    720 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    146 us
  codegen     bench.c       :    430 us
  link        bench_o2      :    981 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 323032 us
  parse       sqlite3.c     : 183610 us
  typecheck   sqlite3.c     :   9879 us
  codegen     sqlite3.c     : 257597 us
  link        sqlite3.so    :   9319 us

RCC -O1:
  preprocess  sqlite3.c     : 308195 us
  parse       sqlite3.c     : 180753 us
  typecheck   sqlite3.c     :   9525 us
  opt         sqlite3.c     :  81106 us
  codegen     sqlite3.c     : 243095 us
  link        sqlite3.so    :   9861 us

RCC -O2:
  preprocess  sqlite3.c     : 294246 us
  parse       sqlite3.c     : 174339 us
  typecheck   sqlite3.c     :   8906 us
  opt         sqlite3.c     : 103661 us
  codegen     sqlite3.c     : 245714 us
  link        sqlite3.so    :   9391 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: | --- | --- |
| RCC       |           17 |    832 | 849 | -   |
| RCC -O1   |           17 |    807 | 824 | -   |
| RCC -O2   |           18 |    824 | 842 | -   |
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
