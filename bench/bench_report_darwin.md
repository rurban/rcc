# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          132 |          899 |       1031 |   118% |
| RCC -O1   |           93 |          805 |        898 |    29% |
| RCC -O2   |           70 |          695 |        765 |    38% |
| TCC       |           27 |          665 |        692 |    70% |
| GCC -O0   |           87 |          600 |        687 |    48% |
| GCC -O2   |          142 |          313 |        455 |    23% |
| Clang -O0 |           71 |          515 |        586 |    28% |
| Clang -O2 |          113 |          354 |        467 |    37% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          151 |         6805 |       6956 |    68% |
| RCC -O1   |          216 |         6488 |       6704 |    34% |
| RCC -O2   |          171 |         5141 |       5312 |    78% |
| TCC       |          129 |         7068 |       7197 |    99% |
| GCC -O0   |          649 |         4127 |       4776 |    24% |
| GCC -O2   |          836 |         1897 |       2733 |    45% |
| Clang -O0 |          507 |         4698 |       5205 |    33% |
| Clang -O2 |          920 |         2651 |       3571 |    53% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2043 us
  parse       bench.c       :    440 us
  typecheck   bench.c       :      8 us
  codegen     bench.c       :    429 us
  link        bench_rcc     :    680 us
  link        bench_rcc     : 106279 us

RCC -O1:
  preprocess  bench.c       :    789 us
  parse       bench.c       :    195 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     64 us
  codegen     bench.c       :    170 us
  link        bench_o1      :    634 us
  link        bench_o1      : 100565 us

RCC -O2:
  preprocess  bench.c       :   1729 us
  parse       bench.c       :    332 us
  typecheck   bench.c       :      8 us
  opt         bench.c       :    101 us
  codegen     bench.c       :    496 us
  link        bench_o2      :    458 us
  link        bench_o2      :  91619 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 467193 us
  parse       sqlite3.c     : 340716 us
  typecheck   sqlite3.c     :  41846 us
  codegen     sqlite3.c     : 277808 us
  link        sqlite3.so    :  24368 us

RCC -O1:
  preprocess  sqlite3.c     : 489750 us
  parse       sqlite3.c     :  94745 us
  typecheck   sqlite3.c     :  34308 us
  opt         sqlite3.c     :  78963 us
  codegen     sqlite3.c     : 297791 us
  link        sqlite3.so    :  33172 us

RCC -O2:
  preprocess  sqlite3.c     : 456647 us
  parse       sqlite3.c     : 104703 us
  typecheck   sqlite3.c     :  32640 us
  opt         sqlite3.c     : 121777 us
  codegen     sqlite3.c     : 479921 us
  link        sqlite3.so    :  37141 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       676 ms |    40% |
| RCC -O1   |       582 ms |    13% |
| RCC -O2   |       628 ms |    10% |
| TCC       |       122 ms |    22% |
| GCC -O0   |      1572 ms |     2% |
| GCC -O2   |     12572 ms |    51% |
| Clang -O0 |      1802 ms |    10% |
| Clang -O2 |     11610 ms |    37% |
