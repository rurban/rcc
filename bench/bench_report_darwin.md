# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           27 |          681 |        708 |     7% |
| RCC -O1   |           28 |          666 |        694 |     3% |
| RCC -O2   |           29 |          360 |        389 |    20% |
| RCC -O3   |           30 |          361 |        391 |    13% |
| TCC       |           23 |          571 |        594 |    73% |
| GCC -O0   |           65 |          486 |        551 |     9% |
| GCC -O2   |           92 |          271 |        363 |    24% |
| Clang -O0 |           53 |          471 |        524 |     9% |
| Clang -O2 |           81 |          269 |        350 |    10% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          140 |         5071 |       5211 |     5% |
| RCC -O1   |          136 |         5038 |       5174 |    30% |
| RCC -O2   |          194 |         2245 |       2439 |    17% |
| RCC -O3   |          207 |         2458 |       2665 |    42% |
| TCC       |          118 |         3780 |       3898 |   134% |
| GCC -O0   |          465 |         3085 |       3550 |    34% |
| GCC -O2   |          784 |         1747 |       2531 |     2% |
| Clang -O0 |          449 |         3190 |       3639 |     6% |
| Clang -O2 |          779 |         1740 |       2519 |    10% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    583 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    137 us
  link        bench_rcc     :  23107 us

RCC -O1:
  preprocess  bench.c       :    590 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     49 us
  codegen     bench.c       :    145 us
  link        bench_o1      :  21963 us

RCC -O2:
  preprocess  bench.c       :    562 us
  parse       bench.c       :    136 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     50 us
  codegen     bench.c       :   1460 us
  link        bench_o2      :  23165 us

RCC -O3:
  preprocess  bench.c       :    536 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     48 us
  codegen     bench.c       :   1454 us
  link        bench_o3      :  22079 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 200346 us
  parse       sqlite3.c     :  53486 us
  typecheck   sqlite3.c     :  12806 us
  codegen     sqlite3.c     : 106114 us
  link        sqlite3.so    :  33284 us

RCC -O1:
  preprocess  sqlite3.c     : 199249 us
  parse       sqlite3.c     :  53080 us
  typecheck   sqlite3.c     :  10819 us
  opt         sqlite3.c     :  43269 us
  codegen     sqlite3.c     : 124988 us
  link        sqlite3.so    :  35057 us

RCC -O2:
  preprocess  sqlite3.c     : 194224 us
  parse       sqlite3.c     :  57095 us
  typecheck   sqlite3.c     :  13051 us
  opt         sqlite3.c     :  54291 us
  codegen     sqlite3.c     : 925739 us
  link        sqlite3.so    :  25840 us

RCC -O3:
  preprocess  sqlite3.c     : 224310 us
  parse       sqlite3.c     :  52809 us
  typecheck   sqlite3.c     :  10659 us
  opt         sqlite3.c     :  53932 us
  codegen     sqlite3.c     : 965956 us
  link        sqlite3.so    :  29446 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       438 ms |    18% |
| RCC -O1   |       434 ms |    10% |
| RCC -O2   |      1146 ms |     0% |
| RCC -O3   |      1136 ms |    26% |
| TCC       |        86 ms |    11% |
| GCC -O0   |       931 ms |     1% |
| GCC -O2   |      8677 ms |     3% |
| Clang -O0 |       915 ms |     0% |
| Clang -O2 |      8541 ms |     0% |
