# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           17 |          786 |        803 |     4% |
| RCC -O1   |           17 |          777 |        794 |     5% |
| RCC -O2   |           17 |          763 |        780 |    41% |
| RCC -O3   |           17 |          797 |        814 |     0% |
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
| RCC       |          302 |         6504 |       6806 |     0% |
| RCC -O1   |          305 |         7336 |       7641 |     0% |
| RCC -O2   |          305 |         6499 |       6804 |     1% |
| RCC -O3   |          308 |         6414 |       6722 |     1% |
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
  preprocess  bench.c       :  10515 us
  parse       bench.c       :    665 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    328 us
  link        bench_rcc     :    792 us

RCC -O1:
  preprocess  bench.c       :  10641 us
  parse       bench.c       :    645 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     94 us
  codegen     bench.c       :    319 us
  link        bench_o1      :    901 us

RCC -O2:
  preprocess  bench.c       :  10620 us
  parse       bench.c       :    620 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     89 us
  codegen     bench.c       :    351 us
  link        bench_o2      :    893 us

RCC -O3:
  preprocess  bench.c       :  10369 us
  parse       bench.c       :    568 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    119 us
  codegen     bench.c       :    408 us
  link        bench_o3      :    892 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 274765 us
  parse       sqlite3.c     : 167896 us
  typecheck   sqlite3.c     :   9272 us
  codegen     sqlite3.c     : 224112 us
  link        sqlite3.so    :   9184 us

RCC -O1:
  preprocess  sqlite3.c     : 268946 us
  parse       sqlite3.c     : 163240 us
  typecheck   sqlite3.c     :   8784 us
  opt         sqlite3.c     :  71441 us
  codegen     sqlite3.c     : 223776 us
  link        sqlite3.so    :   9564 us

RCC -O2:
  preprocess  sqlite3.c     : 274201 us
  parse       sqlite3.c     : 164708 us
  typecheck   sqlite3.c     :   8813 us
  opt         sqlite3.c     :  82987 us
  codegen     sqlite3.c     : 255353 us
  link        sqlite3.so    :  16657 us

RCC -O3:
  preprocess  sqlite3.c     : 266599 us
  parse       sqlite3.c     : 159814 us
  typecheck   sqlite3.c     :   8761 us
  opt         sqlite3.c     :  97969 us
  codegen     sqlite3.c     : 249714 us
  link        sqlite3.so    :   9403 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       799 ms |     0% |
| RCC -O1   |       839 ms |     0% |
| RCC -O2   |       878 ms |     0% |
| RCC -O3   |       897 ms |     0% |
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
