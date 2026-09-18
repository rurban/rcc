# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           63 |          701 |        764 |    20% |
| RCC -O1   |           64 |          702 |        766 |     5% |
| RCC -O2   |           64 |          701 |        765 |     7% |
| TCC       |           28 |          598 |        626 |    64% |
| GCC -O0   |           83 |          503 |        586 |     8% |
| GCC -O2   |          102 |          307 |        409 |     2% |
| Clang -O0 |           74 |          477 |        551 |    20% |
| Clang -O2 |           96 |          291 |        387 |    14% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          134 |         5294 |       5428 |    26% |
| RCC -O1   |          136 |         5437 |       5573 |    25% |
| RCC -O2   |          157 |         5091 |       5248 |    20% |
| TCC       |          121 |         5166 |       5287 |    28% |
| GCC -O0   |          516 |         4059 |       4575 |    35% |
| GCC -O2   |          929 |         2180 |       3109 |    11% |
| Clang -O0 |          505 |         3559 |       4064 |    16% |
| Clang -O2 |          875 |         2067 |       2942 |    18% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    684 us
  parse       bench.c       :    131 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    126 us
  link        bench_rcc     :    107 us
  link        bench_rcc     :  56725 us

RCC -O1:
  preprocess  bench.c       :    562 us
  parse       bench.c       :    127 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     49 us
  codegen     bench.c       :    118 us
  link        bench_o1      :    125 us
  link        bench_o1      :  54776 us

RCC -O2:
  preprocess  bench.c       :    562 us
  parse       bench.c       :    122 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     47 us
  codegen     bench.c       :    121 us
  link        bench_o2      :    162 us
  link        bench_o2      :  52379 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 227974 us
  parse       sqlite3.c     :  51304 us
  typecheck   sqlite3.c     :  11844 us
  codegen     sqlite3.c     : 126748 us
  link        sqlite3.so    :  15227 us

RCC -O1:
  preprocess  sqlite3.c     : 208819 us
  parse       sqlite3.c     :  59515 us
  typecheck   sqlite3.c     :  14766 us
  opt         sqlite3.c     :  51132 us
  codegen     sqlite3.c     : 157200 us
  link        sqlite3.so    :  16153 us

RCC -O2:
  preprocess  sqlite3.c     : 227997 us
  parse       sqlite3.c     :  62114 us
  typecheck   sqlite3.c     :  18898 us
  opt         sqlite3.c     :  55616 us
  codegen     sqlite3.c     : 131283 us
  link        sqlite3.so    :  16856 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       642 ms |    26% |
| RCC -O1   |       616 ms |     1% |
| RCC -O2   |       522 ms |     0% |
| TCC       |       100 ms |    14% |
| GCC -O0   |      1184 ms |     3% |
| GCC -O2   |     11648 ms |    12% |
| Clang -O0 |      1437 ms |     3% |
| Clang -O2 |      9908 ms |     8% |
