# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           59 |          695 |        754 |    32% |
| RCC -O1   |           68 |          690 |        758 |    25% |
| RCC -O2   |           62 |          669 |        731 |    17% |
| TCC       |           28 |          610 |        638 |    75% |
| GCC -O0   |           83 |          499 |        582 |    48% |
| GCC -O2   |          139 |          299 |        438 |    21% |
| Clang -O0 |           60 |          512 |        572 |    15% |
| Clang -O2 |          150 |          307 |        457 |    26% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          130 |         5211 |       5341 |    45% |
| RCC -O1   |          136 |         5659 |       5795 |    16% |
| RCC -O2   |          147 |         5538 |       5685 |   121% |
| TCC       |          221 |         5862 |       6083 |    34% |
| GCC -O0   |          540 |         4154 |       4694 |    26% |
| GCC -O2   |          987 |         2447 |       3434 |    26% |
| Clang -O0 |          556 |         3590 |       4146 |    17% |
| Clang -O2 |          937 |         2255 |       3192 |    41% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    782 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    137 us
  link        bench_rcc     :     87 us
  link        bench_rcc     :  53112 us

RCC -O1:
  preprocess  bench.c       :    583 us
  parse       bench.c       :    125 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    117 us
  link        bench_o1      :    141 us
  link        bench_o1      :  50555 us

RCC -O2:
  preprocess  bench.c       :    796 us
  parse       bench.c       :    131 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    133 us
  link        bench_o2      :     97 us
  link        bench_o2      :  50313 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 266821 us
  parse       sqlite3.c     :  74189 us
  typecheck   sqlite3.c     :  25719 us
  codegen     sqlite3.c     : 140272 us
  link        sqlite3.so    :  16960 us

RCC -O1:
  preprocess  sqlite3.c     : 308718 us
  parse       sqlite3.c     :  86467 us
  typecheck   sqlite3.c     :  15626 us
  opt         sqlite3.c     :  26925 us
  codegen     sqlite3.c     : 127071 us
  link        sqlite3.so    :  18639 us

RCC -O2:
  preprocess  sqlite3.c     : 298381 us
  parse       sqlite3.c     :  62701 us
  typecheck   sqlite3.c     :  12541 us
  opt         sqlite3.c     :  33987 us
  codegen     sqlite3.c     : 120896 us
  link        sqlite3.so    :  16664 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       531 ms |    58% |
| RCC -O1   |       469 ms |     7% |
| RCC -O2   |       471 ms |     3% |
| TCC       |        89 ms |    39% |
| GCC -O0   |      1155 ms |     7% |
| GCC -O2   |      9874 ms |     7% |
| Clang -O0 |      1225 ms |    50% |
| Clang -O2 |     11281 ms |    11% |
