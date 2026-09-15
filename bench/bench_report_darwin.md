# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           86 |          842 |        928 |    36% |
| RCC -O1   |           93 |          820 |        913 |     9% |
| RCC -O2   |           85 |          870 |        955 |    18% |
| TCC       |           44 |          753 |        797 |   115% |
| GCC -O0   |           82 |          613 |        695 |    48% |
| GCC -O2   |          133 |          344 |        477 |    51% |
| Clang -O0 |           91 |          622 |        713 |    31% |
| Clang -O2 |          122 |          339 |        461 |    31% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          188 |         7398 |       7586 |    19% |
| RCC -O1   |          203 |         5687 |       5890 |    68% |
| RCC -O2   |          153 |         4951 |       5104 |   179% |
| TCC       |          128 |         6248 |       6376 |    32% |
| GCC -O0   |          616 |         3900 |       4516 |    27% |
| GCC -O2   |         1067 |         2194 |       3261 |     7% |
| Clang -O0 |          590 |         3688 |       4278 |    15% |
| Clang -O2 |          943 |         2212 |       3155 |     5% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    772 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    165 us
  link        bench_rcc     :    611 us
  link        bench_rcc     :  76391 us

RCC -O1:
  preprocess  bench.c       :    955 us
  parse       bench.c       :    356 us
  typecheck   bench.c       :      8 us
  opt         bench.c       :     52 us
  codegen     bench.c       :    268 us
  link        bench_o1      :    386 us
  link        bench_o1      :  84057 us

RCC -O2:
  preprocess  bench.c       :    749 us
  parse       bench.c       :    136 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    133 us
  link        bench_o2      :    430 us
  link        bench_o2      :  72390 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 290052 us
  parse       sqlite3.c     : 207576 us
  typecheck   sqlite3.c     :  29919 us
  codegen     sqlite3.c     : 202277 us
  link        sqlite3.so    :  28051 us

RCC -O1:
  preprocess  sqlite3.c     : 383309 us
  parse       sqlite3.c     : 102817 us
  typecheck   sqlite3.c     :  36838 us
  opt         sqlite3.c     :  39110 us
  codegen     sqlite3.c     : 193921 us
  link        sqlite3.so    :  20103 us

RCC -O2:
  preprocess  sqlite3.c     : 418430 us
  parse       sqlite3.c     :  90601 us
  typecheck   sqlite3.c     :  25063 us
  opt         sqlite3.c     :  52955 us
  codegen     sqlite3.c     : 254709 us
  link        sqlite3.so    :  21123 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       490 ms |    54% |
| RCC -O1   |       461 ms |     5% |
| RCC -O2   |       458 ms |     4% |
| TCC       |       105 ms |    16% |
| GCC -O0   |      1104 ms |     4% |
| GCC -O2   |     10482 ms |    10% |
| Clang -O0 |      1161 ms |     5% |
| Clang -O2 |     12070 ms |     6% |
