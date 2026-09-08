# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           23 |          693 |        716 |     4% |
| RCC -O1   |           24 |          692 |        716 |     4% |
| RCC -O2   |           23 |          694 |        717 |     5% |
| TCC       |            5 |          485 |        490 |     1% |
| SLIMCC    |           31 |          499 |        530 |     3% |
| XCC       |            8 |          358 |        366 |     1% |
| KEFIR     |          176 |          569 |        745 |     1% |
| KEFIR -O1 |          184 |          306 |        490 |     4% |
| SCC       |           32 |          535 |        567 |    17% |
| LACC      |           24 |          755 |        779 |     4% |
| ANTCC     |           23 |          412 |        435 |     4% |
| CAKE      |           87 |          477 |        564 |     1% |
| CCC       |           32 |          536 |        568 |    16% |
| GCC -O0   |           54 |          478 |        532 |     3% |
| GCC -O2   |          146 |          175 |        321 |     1% |
| Clang -O0 |           78 |          462 |        540 |     2% |
| Clang -O2 |          126 |          176 |        302 |     1% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          363 |         5826 |       6189 |     4% |
| RCC -O1   |          376 |         6605 |       6981 |     0% |
| RCC -O2   |          359 |         6110 |       6469 |     3% |
| TCC       |           67 |         5036 |       5103 |     2% |
| SLIMCC    |          250 |         4872 |       5122 |     0% |
| KEFIR     |         4393 |         5280 |       9673 |     0% |
| KEFIR -O1 |         4819 |         3666 |       8485 |     1% |
| ANTCC     |          184 |         3715 |       3899 |     0% |
| CCC       |          852 |         4452 |       5304 |     1% |
| GCC -O0   |          836 |         4811 |       5647 |     0% |
| GCC -O2   |         1838 |         2517 |       4355 |     1% |
| Clang -O0 |          867 |         4581 |       5448 |     0% |
| Clang -O2 |         1589 |         2552 |       4141 |     0% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   9339 us
  parse       bench.c       :    576 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    387 us
  link        bench_rcc     :  11745 us

RCC -O1:
  preprocess  bench.c       :   9580 us
  parse       bench.c       :    616 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     47 us
  codegen     bench.c       :    300 us
  link        bench_o1      :  10851 us

RCC -O2:
  preprocess  bench.c       :   9716 us
  parse       bench.c       :    570 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     47 us
  codegen     bench.c       :    382 us
  link        bench_o2      :  10241 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 250473 us
  parse       sqlite3.c     : 151179 us
  typecheck   sqlite3.c     :   8226 us
  codegen     sqlite3.c     : 201546 us
  link        sqlite3.so    :   9406 us

RCC -O1:
  preprocess  sqlite3.c     : 242406 us
  parse       sqlite3.c     : 146264 us
  typecheck   sqlite3.c     :   8162 us
  opt         sqlite3.c     :  35959 us
  codegen     sqlite3.c     : 199531 us
  link        sqlite3.so    :   9902 us

RCC -O2:
  preprocess  sqlite3.c     : 243520 us
  parse       sqlite3.c     : 145877 us
  typecheck   sqlite3.c     :   8396 us
  opt         sqlite3.c     :  45575 us
  codegen     sqlite3.c     : 200867 us
  link        sqlite3.so    :  10032 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       720 ms |     0% |
| RCC -O1   |       725 ms |     0% |
| RCC -O2   |       741 ms |     0% |
| TCC       |        98 ms |     0% |
| SLIMCC    |       644 ms |     1% |
| KEFIR     |     18305 ms |     0% |
| KEFIR -O1 |     33077 ms |     0% |
| ANTCC     |       387 ms |     1% |
| CCC       |     12652 ms |     0% |
| GCC -O0   |      4026 ms |     0% |
| GCC -O2   |     25530 ms |     0% |
| Clang -O0 |      1790 ms |     0% |
| Clang -O2 |     19853 ms |     0% |
