# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           46 |          837 |        883 |    28% |
| RCC -O1   |           32 |          814 |        846 |    43% |
| RCC -O2   |           43 |          401 |        444 |    81% |
| RCC -O3   |           41 |          400 |        441 |    21% |
| TCC       |           32 |          730 |        762 |   137% |
| GCC -O0   |          171 |          660 |        831 |    56% |
| GCC -O2   |          187 |          339 |        526 |    89% |
| Clang -O0 |          108 |          578 |        686 |    68% |
| Clang -O2 |          101 |          326 |        427 |    53% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          158 |         6371 |       6529 |    72% |
| RCC -O1   |          252 |         5420 |       5672 |    90% |
| RCC -O2   |          353 |         2435 |       2788 |    77% |
| RCC -O3   |          243 |         2673 |       2916 |    68% |
| TCC       |          118 |         3882 |       4000 |    60% |
| GCC -O0   |          552 |         3423 |       3975 |    25% |
| GCC -O2   |          949 |         2186 |       3135 |    27% |
| Clang -O0 |          500 |         3634 |       4134 |    32% |
| Clang -O2 |          824 |         1944 |       2768 |    31% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    762 us
  parse       bench.c       :    155 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    139 us
  link        bench_rcc     :  26315 us

RCC -O1:
  preprocess  bench.c       :    616 us
  parse       bench.c       :    150 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     58 us
  codegen     bench.c       :    157 us
  link        bench_o1      :  29781 us

RCC -O2:
  preprocess  bench.c       :    517 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     48 us
  codegen     bench.c       :   1456 us
  link        bench_o2      :  22783 us

RCC -O3:
  preprocess  bench.c       :    508 us
  parse       bench.c       :    121 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     47 us
  codegen     bench.c       :   1465 us
  link        bench_o3      :  22394 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 286861 us
  parse       sqlite3.c     :  59822 us
  typecheck   sqlite3.c     :  23021 us
  codegen     sqlite3.c     : 127907 us
  link        sqlite3.so    :  38402 us

RCC -O1:
  preprocess  sqlite3.c     : 227086 us
  parse       sqlite3.c     :  79013 us
  typecheck   sqlite3.c     :  20198 us
  opt         sqlite3.c     :  55510 us
  codegen     sqlite3.c     : 191886 us
  link        sqlite3.so    :  34020 us

RCC -O2:
  preprocess  sqlite3.c     : 220864 us
  parse       sqlite3.c     :  51254 us
  typecheck   sqlite3.c     :  12913 us
  opt         sqlite3.c     :  59213 us
  codegen     sqlite3.c     : 1306911 us
  link        sqlite3.so    :  32898 us

RCC -O3:
  preprocess  sqlite3.c     : 275265 us
  parse       sqlite3.c     :  74453 us
  typecheck   sqlite3.c     :  16304 us
  opt         sqlite3.c     :  88990 us
  codegen     sqlite3.c     : 1448051 us
  link        sqlite3.so    :  49066 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       522 ms |    67% |
| RCC -O1   |       485 ms |     6% |
| RCC -O2   |      1405 ms |     4% |
| RCC -O3   |      1336 ms |     3% |
| TCC       |       103 ms |    19% |
| GCC -O0   |      1076 ms |     2% |
| GCC -O2   |     10051 ms |     7% |
| Clang -O0 |       939 ms |     4% |
| Clang -O2 |      8693 ms |     4% |
