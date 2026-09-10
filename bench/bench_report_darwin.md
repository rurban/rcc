# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           53 |          630 |        683 |    16% |
| RCC -O1   |           53 |          651 |        704 |    15% |
| RCC -O2   |           70 |          644 |        714 |    17% |
| TCC       |           23 |          517 |        540 |    73% |
| GCC -O0   |           61 |          440 |        501 |    19% |
| GCC -O2   |           90 |          269 |        359 |    20% |
| Clang -O0 |           62 |          437 |        499 |    19% |
| Clang -O2 |           98 |          272 |        370 |     7% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          121 |         5083 |       5204 |    91% |
| RCC -O1   |          121 |         4618 |       4739 |    79% |
| RCC -O2   |          120 |         3935 |       4055 |    37% |
| TCC       |           99 |         3713 |       3812 |    66% |
| GCC -O0   |          457 |         3084 |       3541 |    13% |
| GCC -O2   |          830 |         1740 |       2570 |     7% |
| Clang -O0 |          463 |         3072 |       3535 |     8% |
| Clang -O2 |          782 |         1765 |       2547 |     4% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    881 us
  parse       bench.c       :    213 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    162 us
  link        bench_rcc     :    222 us
  link        bench_rcc     :  55475 us

RCC -O1:
  preprocess  bench.c       :    560 us
  parse       bench.c       :    124 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    118 us
  link        bench_o1      :    119 us
  link        bench_o1      :  50679 us

RCC -O2:
  preprocess  bench.c       :    545 us
  parse       bench.c       :    141 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    117 us
  link        bench_o2      :    113 us
  link        bench_o2      :  47162 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 211731 us
  parse       sqlite3.c     :  47441 us
  typecheck   sqlite3.c     :  13345 us
  codegen     sqlite3.c     : 103776 us
  link        sqlite3.so    :  28860 us

RCC -O1:
  preprocess  sqlite3.c     : 200695 us
  parse       sqlite3.c     :  50809 us
  typecheck   sqlite3.c     :  11063 us
  opt         sqlite3.c     :  22090 us
  codegen     sqlite3.c     : 119566 us
  link        sqlite3.so    :  14102 us

RCC -O2:
  preprocess  sqlite3.c     : 193032 us
  parse       sqlite3.c     :  52524 us
  typecheck   sqlite3.c     :  11888 us
  opt         sqlite3.c     :  29198 us
  codegen     sqlite3.c     : 127570 us
  link        sqlite3.so    :  15172 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       432 ms |    56% |
| RCC -O1   |       396 ms |     2% |
| RCC -O2   |       399 ms |     1% |
| TCC       |        85 ms |    43% |
| GCC -O0   |       916 ms |     3% |
| GCC -O2   |      8685 ms |     1% |
| Clang -O0 |       913 ms |     0% |
| Clang -O2 |      8616 ms |     2% |
