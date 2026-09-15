# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          110 |          884 |        994 |    16% |
| RCC -O1   |           87 |          819 |        906 |    68% |
| RCC -O2   |          109 |          883 |        992 |    21% |
| TCC       |           53 |          646 |        699 |   133% |
| GCC -O0   |           99 |          529 |        628 |    36% |
| GCC -O2   |          120 |          310 |        430 |    44% |
| Clang -O0 |           63 |          492 |        555 |    19% |
| Clang -O2 |          102 |          309 |        411 |     7% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          182 |         5000 |       5182 |    29% |
| RCC -O1   |          135 |         5281 |       5416 |    46% |
| RCC -O2   |          152 |         4850 |       5002 |    53% |
| TCC       |          118 |         4936 |       5054 |    51% |
| GCC -O0   |          515 |         3505 |       4020 |    30% |
| GCC -O2   |          973 |         2013 |       2986 |     5% |
| Clang -O0 |          614 |         4179 |       4793 |    28% |
| Clang -O2 |         1066 |         2129 |       3195 |    41% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2200 us
  parse       bench.c       :    168 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    148 us
  link        bench_rcc     :    330 us
  link        bench_rcc     :  79206 us

RCC -O1:
  preprocess  bench.c       :    789 us
  parse       bench.c       :    220 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     34 us
  codegen     bench.c       :    176 us
  link        bench_o1      :    457 us
  link        bench_o1      :  72287 us

RCC -O2:
  preprocess  bench.c       :    958 us
  parse       bench.c       :    243 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     30 us
  codegen     bench.c       :    206 us
  link        bench_o2      :    220 us
  link        bench_o2      :  68711 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 310698 us
  parse       sqlite3.c     :  89174 us
  typecheck   sqlite3.c     :  21245 us
  codegen     sqlite3.c     : 215425 us
  link        sqlite3.so    :  23278 us

RCC -O1:
  preprocess  sqlite3.c     : 342593 us
  parse       sqlite3.c     : 104523 us
  typecheck   sqlite3.c     :  17652 us
  opt         sqlite3.c     :  46199 us
  codegen     sqlite3.c     : 208424 us
  link        sqlite3.so    :  22634 us

RCC -O2:
  preprocess  sqlite3.c     : 435253 us
  parse       sqlite3.c     : 161153 us
  typecheck   sqlite3.c     :  20001 us
  opt         sqlite3.c     :  70349 us
  codegen     sqlite3.c     : 284137 us
  link        sqlite3.so    :  27254 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       561 ms |    36% |
| RCC -O1   |       518 ms |     7% |
| RCC -O2   |       568 ms |     4% |
| TCC       |       110 ms |    20% |
| GCC -O0   |      1150 ms |    11% |
| GCC -O2   |      9434 ms |    31% |
| Clang -O0 |       985 ms |    16% |
| Clang -O2 |      9180 ms |     9% |
