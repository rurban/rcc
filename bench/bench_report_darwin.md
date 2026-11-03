# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           73 |          953 |       1026 |    73% |
| RCC -O1   |           43 |          888 |        931 |    83% |
| RCC -O2   |           42 |          780 |        822 |    42% |
| RCC -O3   |           45 |          819 |        864 |    75% |
| TCC       |           34 |          765 |        799 |   188% |
| GCC -O0   |          121 |          615 |        736 |    24% |
| GCC -O2   |          162 |          336 |        498 |   112% |
| Clang -O0 |           97 |          691 |        788 |    42% |
| Clang -O2 |          203 |          367 |        570 |   121% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          246 |         7584 |       7830 |    70% |
| RCC -O1   |          326 |         7986 |       8312 |    35% |
| RCC -O2   |          226 |         7136 |       7362 |   144% |
| RCC -O3   |          351 |         5364 |       5715 |    44% |
| TCC       |          173 |         5800 |       5973 |    67% |
| GCC -O0   |          747 |         4778 |       5525 |     8% |
| GCC -O2   |         1474 |         2885 |       4359 |    16% |
| Clang -O0 |          796 |         5664 |       6460 |    24% |
| Clang -O2 |         1365 |         2462 |       3827 |    13% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2020 us
  parse       bench.c       :    414 us
  typecheck   bench.c       :     17 us
  codegen     bench.c       :    618 us
  link        bench_rcc     :  65677 us

RCC -O1:
  preprocess  bench.c       :   1683 us
  parse       bench.c       :    319 us
  typecheck   bench.c       :      9 us
  opt         bench.c       :    111 us
  codegen     bench.c       :    308 us
  link        bench_o1      :  51349 us

RCC -O2:
  preprocess  bench.c       :   2422 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     49 us
  codegen     bench.c       :    286 us
  link        bench_o2      :  64557 us

RCC -O3:
  preprocess  bench.c       :    678 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :    138 us
  codegen     bench.c       :    166 us
  link        bench_o3      :  48727 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 791605 us
  parse       sqlite3.c     : 315773 us
  typecheck   sqlite3.c     :  27156 us
  codegen     sqlite3.c     : 386343 us
  link        sqlite3.so    : 101134 us

RCC -O1:
  preprocess  sqlite3.c     : 721367 us
  parse       sqlite3.c     : 142613 us
  typecheck   sqlite3.c     :  42564 us
  opt         sqlite3.c     : 113652 us
  codegen     sqlite3.c     : 280332 us
  link        sqlite3.so    : 101856 us

RCC -O2:
  preprocess  sqlite3.c     : 647236 us
  parse       sqlite3.c     : 131188 us
  typecheck   sqlite3.c     :  31596 us
  opt         sqlite3.c     : 175197 us
  codegen     sqlite3.c     : 348683 us
  link        sqlite3.so    :  77077 us

RCC -O3:
  preprocess  sqlite3.c     : 578327 us
  parse       sqlite3.c     : 119499 us
  typecheck   sqlite3.c     :  26326 us
  opt         sqlite3.c     : 152180 us
  codegen     sqlite3.c     : 305093 us
  link        sqlite3.so    :  96233 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       645 ms |    20% |
| RCC -O1   |       570 ms |    17% |
| RCC -O2   |       647 ms |     5% |
| RCC -O3   |       657 ms |     7% |
| TCC       |       115 ms |    11% |
| GCC -O0   |      1573 ms |     3% |
| GCC -O2   |     10332 ms |    23% |
| Clang -O0 |      1551 ms |    11% |
| Clang -O2 |     17327 ms |     7% |
