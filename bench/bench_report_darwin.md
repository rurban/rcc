# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           28 |          665 |        693 |     9% |
| RCC -O1   |           32 |          718 |        750 |     9% |
| RCC -O2   |           37 |          388 |        425 |    16% |
| RCC -O3   |           47 |          358 |        405 |    18% |
| TCC       |           34 |          571 |        605 |    38% |
| GCC -O0   |           64 |          474 |        538 |    12% |
| GCC -O2   |          112 |          293 |        405 |    11% |
| Clang -O0 |           58 |          502 |        560 |    22% |
| Clang -O2 |           86 |          289 |        375 |    32% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          159 |         3904 |       4063 |    11% |
| RCC -O1   |          119 |         3897 |       4016 |    15% |
| RCC -O2   |          161 |         1995 |       2156 |     1% |
| RCC -O3   |          160 |         1999 |       2159 |     2% |
| TCC       |          113 |         4032 |       4145 |    14% |
| GCC -O0   |          427 |         3239 |       3666 |    14% |
| GCC -O2   |          908 |         1883 |       2791 |    16% |
| Clang -O0 |          477 |         2877 |       3354 |    11% |
| Clang -O2 |          692 |         1687 |       2379 |     8% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    584 us
  parse       bench.c       :    135 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    132 us
  link        bench_rcc     :  20161 us

RCC -O1:
  preprocess  bench.c       :    488 us
  parse       bench.c       :    127 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     54 us
  codegen     bench.c       :    134 us
  link        bench_o1      :  20989 us

RCC -O2:
  preprocess  bench.c       :    485 us
  parse       bench.c       :    132 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     52 us
  codegen     bench.c       :   1395 us
  link        bench_o2      :  20972 us

RCC -O3:
  preprocess  bench.c       :    507 us
  parse       bench.c       :    124 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     51 us
  codegen     bench.c       :   1388 us
  link        bench_o3      :  21046 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 165141 us
  parse       sqlite3.c     :  45475 us
  typecheck   sqlite3.c     :  12181 us
  codegen     sqlite3.c     :  95742 us
  link        sqlite3.so    :  25919 us

RCC -O1:
  preprocess  sqlite3.c     : 152753 us
  parse       sqlite3.c     :  42794 us
  typecheck   sqlite3.c     :  10794 us
  opt         sqlite3.c     :  38606 us
  codegen     sqlite3.c     :  96628 us
  link        sqlite3.so    :  24267 us

RCC -O2:
  preprocess  sqlite3.c     : 153747 us
  parse       sqlite3.c     :  47520 us
  typecheck   sqlite3.c     :  12573 us
  opt         sqlite3.c     :  48132 us
  codegen     sqlite3.c     : 877811 us
  link        sqlite3.so    :  23976 us

RCC -O3:
  preprocess  sqlite3.c     : 166646 us
  parse       sqlite3.c     :  46290 us
  typecheck   sqlite3.c     :  11681 us
  opt         sqlite3.c     :  44313 us
  codegen     sqlite3.c     : 821031 us
  link        sqlite3.so    :  27235 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       409 ms |    33% |
| RCC -O1   |       469 ms |    10% |
| RCC -O2   |      1214 ms |     1% |
| RCC -O3   |      1197 ms |     3% |
| TCC       |        91 ms |     2% |
| GCC -O0   |       953 ms |     2% |
| GCC -O2   |      9612 ms |     3% |
| Clang -O0 |       894 ms |    13% |
| Clang -O2 |      8570 ms |     9% |
