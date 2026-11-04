# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           26 |          596 |        622 |     7% |
| RCC -O1   |           26 |          612 |        638 |     2% |
| RCC -O2   |           25 |          593 |        618 |    24% |
| RCC -O3   |           35 |          646 |        681 |     7% |
| TCC       |           29 |          577 |        606 |   162% |
| GCC -O0   |           68 |          441 |        509 |    25% |
| GCC -O2   |           84 |          269 |        353 |    26% |
| Clang -O0 |           56 |          451 |        507 |     7% |
| Clang -O2 |           82 |          285 |        367 |    13% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          149 |         4246 |       4395 |     6% |
| RCC -O1   |          127 |         4033 |       4160 |    14% |
| RCC -O2   |          128 |         3632 |       3760 |     9% |
| RCC -O3   |          153 |         3564 |       3717 |     9% |
| TCC       |           98 |         3350 |       3448 |     2% |
| GCC -O0   |          395 |         3152 |       3547 |    12% |
| GCC -O2   |          932 |         1816 |       2748 |    41% |
| Clang -O0 |          491 |         3123 |       3614 |    11% |
| Clang -O2 |          831 |         1824 |       2655 |     8% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    546 us
  parse       bench.c       :    122 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    126 us
  link        bench_rcc     :  19358 us

RCC -O1:
  preprocess  bench.c       :    445 us
  parse       bench.c       :    117 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     44 us
  codegen     bench.c       :    111 us
  link        bench_o1      :  18896 us

RCC -O2:
  preprocess  bench.c       :    432 us
  parse       bench.c       :    106 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     45 us
  codegen     bench.c       :    125 us
  link        bench_o2      :  20381 us

RCC -O3:
  preprocess  bench.c       :    424 us
  parse       bench.c       :    106 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     78 us
  codegen     bench.c       :    124 us
  link        bench_o3      :  19938 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 148214 us
  parse       sqlite3.c     :  40101 us
  typecheck   sqlite3.c     :  10727 us
  codegen     sqlite3.c     :  90309 us
  link        sqlite3.so    :  26261 us

RCC -O1:
  preprocess  sqlite3.c     : 160342 us
  parse       sqlite3.c     :  43765 us
  typecheck   sqlite3.c     :  12904 us
  opt         sqlite3.c     :  40155 us
  codegen     sqlite3.c     :  98911 us
  link        sqlite3.so    :  28806 us

RCC -O2:
  preprocess  sqlite3.c     : 158747 us
  parse       sqlite3.c     :  43301 us
  typecheck   sqlite3.c     :  11645 us
  opt         sqlite3.c     :  43234 us
  codegen     sqlite3.c     : 112540 us
  link        sqlite3.so    :  27259 us

RCC -O3:
  preprocess  sqlite3.c     : 161771 us
  parse       sqlite3.c     :  44814 us
  typecheck   sqlite3.c     :  11519 us
  opt         sqlite3.c     :  55963 us
  codegen     sqlite3.c     : 110060 us
  link        sqlite3.so    :  26892 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       439 ms |    29% |
| RCC -O1   |       410 ms |    10% |
| RCC -O2   |       437 ms |     2% |
| RCC -O3   |       436 ms |     3% |
| TCC       |        84 ms |     2% |
| GCC -O0   |       831 ms |     2% |
| GCC -O2   |      8649 ms |     9% |
| Clang -O0 |      1060 ms |     1% |
| Clang -O2 |      9435 ms |     6% |
