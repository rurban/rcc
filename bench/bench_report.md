# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           13 |          663 |        676 |    23% |
| RCC -O1   |           13 |          661 |        674 |    30% |
| RCC -O2   |           13 |          661 |        674 |    30% |
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
| RCC       |          273 |         7717 |       7990 |     1% |
| RCC -O1   |          274 |         8290 |       8564 |     0% |
| RCC -O2   |          273 |         7579 |       7852 |     0% |
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
  preprocess  bench.c       :   7109 us
  parse       bench.c       :    642 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    357 us
  link        bench_rcc     :    624 us

RCC -O1:
  preprocess  bench.c       :   8814 us
  parse       bench.c       :    843 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :    104 us
  codegen     bench.c       :    414 us
  link        bench_o1      :    800 us

RCC -O2:
  preprocess  bench.c       :   7131 us
  parse       bench.c       :    633 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     85 us
  codegen     bench.c       :    356 us
  link        bench_o2      :    610 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 229620 us
  parse       sqlite3.c     : 176853 us
  typecheck   sqlite3.c     :   9562 us
  codegen     sqlite3.c     : 207675 us
  link        sqlite3.so    :  12570 us

RCC -O1:
  preprocess  sqlite3.c     : 228929 us
  parse       sqlite3.c     : 176934 us
  typecheck   sqlite3.c     :   9554 us
  opt         sqlite3.c     :  62637 us
  codegen     sqlite3.c     : 225489 us
  link        sqlite3.so    :  20055 us

RCC -O2:
  preprocess  sqlite3.c     : 226887 us
  parse       sqlite3.c     : 179131 us
  typecheck   sqlite3.c     :   9574 us
  opt         sqlite3.c     :  73770 us
  codegen     sqlite3.c     : 207704 us
  link        sqlite3.so    :  19008 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       769 ms |     0% |
| RCC -O1   |       799 ms |     0% |
| RCC -O2   |       815 ms |     0% |
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
