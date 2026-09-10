# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           15 |          891 |        906 |    20% |
| RCC -O1   |           16 |          946 |        962 |    12% |
| RCC -O2   |           14 |          863 |        877 |    21% |
| TCC       |            6 |          647 |        653 |    16% |
| SLIMCC    |           50 |          619 |        669 |     8% |
| XCC       |           10 |          453 |        463 |    30% |
| KEFIR     |          238 |          713 |        951 |     9% |
| KEFIR -O1 |          253 |          385 |        638 |     7% |
| SCC       |           41 |          692 |        733 |    48% |
| LACC      |           25 |         1936 |       1961 |    16% |
| ANTCC     |           62 |         1016 |       1078 |    12% |
| CAKE      |          241 |         1201 |       1442 |     5% |
| BCC       |           64 |         1426 |       1490 |     3% |
| CCC       |           77 |         1377 |       1454 |    16% |
| GCC -O0   |          136 |         1207 |       1343 |     9% |
| GCC -O2   |          382 |          411 |        793 |     4% |
| Clang -O0 |          202 |         1131 |       1333 |     8% |
| Clang -O2 |          325 |          445 |        770 |    12% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          679 |        18745 |      19424 |     3% |
| RCC -O1   |          741 |        25286 |      26027 |    17% |
| RCC -O2   |          738 |        21078 |      21816 |    85% |
| TCC       |          198 |        17640 |      17838 |     9% |
| SLIMCC    |          764 |        18737 |      19501 |     6% |
| KEFIR     |        16732 |        17038 |      33770 |     2% |
| KEFIR -O1 |        15009 |        13219 |      28228 |    11% |
| ANTCC     |          525 |        14374 |      14899 |    16% |
| CCC       |         2534 |         7872 |      10406 |     2% |
| BCC       |         4404 |         7870 |      12274 |     7% |
| GCC -O0   |         1173 |         7923 |       9096 |     9% |
| GCC -O2   |         2745 |         4583 |       7328 |     3% |
| Clang -O0 |         1155 |         7281 |       8436 |     3% |
| Clang -O2 |         2197 |         4612 |       6809 |     3% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  14934 us
  parse       bench.c       :   1013 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    473 us
  link        bench_rcc     :   1363 us

RCC -O1:
  preprocess  bench.c       :  14335 us
  parse       bench.c       :    894 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     86 us
  codegen     bench.c       :    635 us
  link        bench_o1      :   1462 us

RCC -O2:
  preprocess  bench.c       :  14387 us
  parse       bench.c       :    896 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :    153 us
  codegen     bench.c       :    463 us
  link        bench_o2      :   1413 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 295924 us
  parse       sqlite3.c     : 194023 us
  typecheck   sqlite3.c     :   9445 us
  codegen     sqlite3.c     : 260666 us
  link        sqlite3.so    :  10605 us

RCC -O1:
  preprocess  sqlite3.c     : 279247 us
  parse       sqlite3.c     : 206921 us
  typecheck   sqlite3.c     :  12600 us
  opt         sqlite3.c     :  47265 us
  codegen     sqlite3.c     : 252397 us
  link        sqlite3.so    :  12930 us

RCC -O2:
  preprocess  sqlite3.c     : 270685 us
  parse       sqlite3.c     : 185595 us
  typecheck   sqlite3.c     :  11837 us
  opt         sqlite3.c     :  58304 us
  codegen     sqlite3.c     : 257422 us
  link        sqlite3.so    :  10888 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       954 ms |     2% |
| RCC -O1   |      1091 ms |     4% |
| RCC -O2   |      1089 ms |     9% |
| TCC       |       155 ms |     8% |
| SLIMCC    |       943 ms |    13% |
| KEFIR     |     28949 ms |     0% |
| KEFIR -O1 |     53935 ms |     3% |
| ANTCC     |       589 ms |     2% |
| CCC       |     20369 ms |     0% |
| GCC -O0   |      6632 ms |     0% |
| GCC -O2   |     60761 ms |    20% |
| Clang -O0 |      4619 ms |    25% |
| Clang -O2 |     64464 ms |     4% |
