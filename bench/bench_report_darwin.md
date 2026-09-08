# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           67 |          618 |        685 |    15% |
| RCC -O1   |           64 |          665 |        729 |    28% |
| RCC -O2   |           52 |          655 |        707 |    17% |
| TCC       |           34 |          570 |        604 |    32% |
| GCC -O0   |           64 |          486 |        550 |    42% |
| GCC -O2   |          116 |          305 |        421 |    67% |
| Clang -O0 |           64 |          475 |        539 |    60% |
| Clang -O2 |           87 |          294 |        381 |    22% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          127 |         5452 |       5579 |    37% |
| RCC -O1   |          247 |         5506 |       5753 |    85% |
| RCC -O2   |          181 |         4283 |       4464 |   161% |
| TCC       |          106 |         4554 |       4660 |    62% |
| GCC -O0   |          550 |         3451 |       4001 |    41% |
| GCC -O2   |         1008 |         2270 |       3278 |    29% |
| Clang -O0 |          568 |         3645 |       4213 |    19% |
| Clang -O2 |          900 |         2100 |       3000 |    13% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    835 us
  parse       bench.c       :    196 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    183 us
  link        bench_rcc     :     54 us
  link        bench_rcc     :  52638 us

RCC -O1:
  preprocess  bench.c       :    665 us
  parse       bench.c       :    187 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    152 us
  link        bench_o1      :    101 us
  link        bench_o1      :  53107 us

RCC -O2:
  preprocess  bench.c       :    618 us
  parse       bench.c       :    167 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    152 us
  link        bench_o2      :    152 us
  link        bench_o2      :  50389 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 236802 us
  parse       sqlite3.c     :  53218 us
  typecheck   sqlite3.c     :  12305 us
  codegen     sqlite3.c     : 111883 us
  link        sqlite3.so    :  15784 us

RCC -O1:
  preprocess  sqlite3.c     : 224371 us
  parse       sqlite3.c     :  58222 us
  typecheck   sqlite3.c     :  13122 us
  opt         sqlite3.c     : 145915 us
  codegen     sqlite3.c     : 108595 us
  link        sqlite3.so    :  14930 us

RCC -O2:
  preprocess  sqlite3.c     : 208360 us
  parse       sqlite3.c     :  60847 us
  typecheck   sqlite3.c     :  13047 us
  opt         sqlite3.c     : 183911 us
  codegen     sqlite3.c     : 119296 us
  link        sqlite3.so    :  15506 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       853 ms |    38% |
| RCC -O1   |       878 ms |     4% |
| RCC -O2   |       914 ms |     5% |
| TCC       |       152 ms |     1% |
| GCC -O0   |      1355 ms |     5% |
| GCC -O2   |     14220 ms |    11% |
| Clang -O0 |      1095 ms |     6% |
| Clang -O2 |     12120 ms |     9% |
