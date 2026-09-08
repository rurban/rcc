# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           31 |          836 |        867 |
| RCC -O1   |           33 |          956 |        989 |
| RCC -O2   |           56 |         1099 |       1155 |
| TCC       |           19 |          735 |        754 |
| SLIMCC    |           86 |          555 |        641 |
| XCC       |           38 |          397 |        435 |
| KEFIR     |          272 |          632 |        904 |
| KEFIR -O1 |          246 |          364 |        610 |
| SCC       |          177 |          633 |        810 |
| LACC      |           51 |          886 |        937 |
| ANTCC     |           36 |          486 |        522 |
| CAKE      |          128 |          561 |        689 |
| CCC       |           44 |          705 |        749 |
| GCC -O0   |           68 |          621 |        689 |
| GCC -O2   |          193 |          222 |        415 |
| Clang -O0 |          103 |          583 |        686 |
| Clang -O2 |          206 |          226 |        432 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          478 |         6795 |       7273 |
| RCC -O1   |          502 |         8232 |       8734 |
| RCC -O2   |          395 |         7268 |       7663 |
| TCC       |           89 |         5623 |       5712 |
| SLIMCC    |          297 |         5533 |       5830 |
| KEFIR     |         6133 |         5869 |      12002 |
| KEFIR -O1 |         5475 |         4151 |       9626 |
| ANTCC     |          214 |         4068 |       4282 |
| CCC       |          956 |         5130 |       6086 |
| GCC -O0   |         1024 |         6189 |       7213 |
| GCC -O2   |         2288 |         3070 |       5358 |
| Clang -O0 |          963 |         5000 |       5963 |
| Clang -O2 |         1779 |         2878 |       4657 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  10317 us
  parse       bench.c       :    681 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    301 us
  link        bench_rcc     :  10697 us

RCC -O1:
  preprocess  bench.c       :  12672 us
  parse       bench.c       :    766 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    300 us
  link        bench_o1      :  11713 us

RCC -O2:
  preprocess  bench.c       :   9969 us
  parse       bench.c       :    629 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    416 us
  link        bench_o2      :  12398 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 272950 us
  parse       sqlite3.c     : 168452 us
  typecheck   sqlite3.c     :  10016 us
  codegen     sqlite3.c     : 224721 us
  link        sqlite3.so    :  10036 us

RCC -O1:
  preprocess  sqlite3.c     : 272204 us
  parse       sqlite3.c     : 160701 us
  typecheck   sqlite3.c     :   8606 us
  opt         sqlite3.c     : 247596 us
  codegen     sqlite3.c     : 290222 us
  link        sqlite3.so    :  11389 us

RCC -O2:
  preprocess  sqlite3.c     : 292123 us
  parse       sqlite3.c     : 177249 us
  typecheck   sqlite3.c     :   8868 us
  opt         sqlite3.c     : 273212 us
  codegen     sqlite3.c     : 245625 us
  link        sqlite3.so    :  13417 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1192 ms |
| RCC -O1   |      1196 ms |
| RCC -O2   |      1214 ms |
| TCC       |       116 ms |
| SLIMCC    |       706 ms |
| KEFIR     |     20176 ms |
| KEFIR -O1 |     37452 ms |
| ANTCC     |       419 ms |
| CCC       |     16261 ms |
| GCC -O0   |      5868 ms |
| GCC -O2   |     32589 ms |
| Clang -O0 |      2345 ms |
| Clang -O2 |     23833 ms |
