# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           14 |          665 |        679 |    14% |
| RCC -O1   |           13 |          664 |        677 |    46% |
| RCC -O2   |           13 |          665 |        678 |    46% |
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
| RCC       |          279 |         7721 |       8000 |     2% |
| RCC -O1   |          274 |         8284 |       8558 |     0% |
| RCC -O2   |          274 |         7588 |       7862 |     0% |
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
  preprocess  bench.c       :   7115 us
  parse       bench.c       :    631 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    370 us
  link        bench_rcc     :    643 us

RCC -O1:
  preprocess  bench.c       :   8773 us
  parse       bench.c       :    839 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :    107 us
  codegen     bench.c       :    450 us
  link        bench_o1      :    711 us

RCC -O2:
  preprocess  bench.c       :   7448 us
  parse       bench.c       :    633 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     85 us
  codegen     bench.c       :    346 us
  link        bench_o2      :    626 us

RCC -O3:
  preprocess  bench.c       :   8687 us
  parse       bench.c       :    842 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :    140 us
  codegen     bench.c       :    406 us
  link        bench_o3      :    644 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 228946 us
  parse       sqlite3.c     : 175064 us
  typecheck   sqlite3.c     :   9551 us
  codegen     sqlite3.c     : 208368 us
  link        sqlite3.so    :  12577 us

RCC -O1:
  preprocess  sqlite3.c     : 226361 us
  parse       sqlite3.c     : 174652 us
  typecheck   sqlite3.c     :   9567 us
  opt         sqlite3.c     :  62937 us
  codegen     sqlite3.c     : 204708 us
  link        sqlite3.so    :  19380 us

RCC -O2:
  preprocess  sqlite3.c     : 225151 us
  parse       sqlite3.c     : 174240 us
  typecheck   sqlite3.c     :   9553 us
  opt         sqlite3.c     :  73966 us
  codegen     sqlite3.c     : 207920 us
  link        sqlite3.so    :  19290 us

RCC -O3:
  preprocess  sqlite3.c     : 225722 us
  parse       sqlite3.c     : 173958 us
  typecheck   sqlite3.c     :   9547 us
  opt         sqlite3.c     :  90155 us
  codegen     sqlite3.c     : 208199 us
  link        sqlite3.so    :  19244 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       763 ms |     0% |
| RCC -O1   |       793 ms |     0% |
| RCC -O2   |       809 ms |     0% |
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
