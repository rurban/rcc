# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          104 |          896 |       1000 |   121% |
| RCC -O1   |           92 |          950 |       1042 |    34% |
| RCC -O2   |           77 |          897 |        974 |    22% |
| TCC       |           43 |          762 |        805 |    69% |
| GCC -O0   |           81 |          651 |        732 |    39% |
| GCC -O2   |          167 |          352 |        519 |     9% |
| Clang -O0 |           87 |          647 |        734 |    55% |
| Clang -O2 |          157 |          379 |        536 |    17% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          203 |         7727 |       7930 |    31% |
| RCC -O1   |          199 |         7066 |       7265 |    44% |
| RCC -O2   |          223 |         5937 |       6160 |   123% |
| TCC       |          141 |         6339 |       6480 |   132% |
| GCC -O0   |          603 |         4190 |       4793 |    46% |
| GCC -O2   |         1051 |         2595 |       3646 |    16% |
| Clang -O0 |          599 |         4331 |       4930 |    16% |
| Clang -O2 |         1172 |         2750 |       3922 |     6% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    889 us
  parse       bench.c       :    172 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    172 us
  link        bench_rcc     :    678 us
  link        bench_rcc     : 116398 us

RCC -O1:
  preprocess  bench.c       :    695 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     46 us
  codegen     bench.c       :    152 us
  link        bench_o1      :    191 us
  link        bench_o1      : 188175 us

RCC -O2:
  preprocess  bench.c       :    919 us
  parse       bench.c       :    198 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     45 us
  codegen     bench.c       :    153 us
  link        bench_o2      :    183 us
  link        bench_o2      :  94027 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 609636 us
  parse       sqlite3.c     : 237765 us
  typecheck   sqlite3.c     :  27884 us
  codegen     sqlite3.c     : 240035 us
  link        sqlite3.so    :  31119 us

RCC -O1:
  preprocess  sqlite3.c     : 496847 us
  parse       sqlite3.c     : 103193 us
  typecheck   sqlite3.c     :  53139 us
  opt         sqlite3.c     :  65483 us
  codegen     sqlite3.c     : 201338 us
  link        sqlite3.so    :  17975 us

RCC -O2:
  preprocess  sqlite3.c     : 354204 us
  parse       sqlite3.c     :  92193 us
  typecheck   sqlite3.c     :  20431 us
  opt         sqlite3.c     :  66975 us
  codegen     sqlite3.c     : 917326 us
  link        sqlite3.so    :  26178 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       672 ms |    51% |
| RCC -O1   |       570 ms |    21% |
| RCC -O2   |       623 ms |     9% |
| TCC       |       128 ms |    32% |
| GCC -O0   |      1370 ms |     8% |
| GCC -O2   |     13897 ms |     7% |
| Clang -O0 |      1514 ms |     4% |
| Clang -O2 |     12696 ms |     7% |
