# Darwin RCC Benchmark Results

_Generated: October 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           28 |          675 |        703 |    32% |
| RCC -O1   |           33 |          675 |        708 |     7% |
| RCC -O2   |           37 |          650 |        687 |    40% |
| TCC       |           24 |          551 |        575 |   112% |
| GCC -O0   |           60 |          463 |        523 |    26% |
| GCC -O2   |          114 |          277 |        391 |    36% |
| Clang -O0 |           55 |          446 |        501 |    14% |
| Clang -O2 |           84 |          288 |        372 |     5% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          133 |         4960 |       5093 |    12% |
| RCC -O1   |          144 |         5045 |       5189 |    31% |
| RCC -O2   |          173 |         4910 |       5083 |    42% |
| TCC       |          100 |         4916 |       5016 |    23% |
| GCC -O0   |          504 |         3663 |       4167 |    35% |
| GCC -O2   |          810 |         1949 |       2759 |    14% |
| Clang -O0 |          464 |         4029 |       4493 |    26% |
| Clang -O2 |          969 |         1875 |       2844 |    22% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    918 us
  parse       bench.c       :    232 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    188 us
  link        bench_rcc     :  31748 us

RCC -O1:
  preprocess  bench.c       :    725 us
  parse       bench.c       :    209 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     72 us
  codegen     bench.c       :    205 us
  link        bench_o1      :  30144 us

RCC -O2:
  preprocess  bench.c       :    688 us
  parse       bench.c       :    192 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     62 us
  codegen     bench.c       :    156 us
  link        bench_o2      :  27827 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 286039 us
  parse       sqlite3.c     :  66106 us
  typecheck   sqlite3.c     :  14792 us
  codegen     sqlite3.c     : 114795 us
  link        sqlite3.so    :  44493 us

RCC -O1:
  preprocess  sqlite3.c     : 203384 us
  parse       sqlite3.c     :  56417 us
  typecheck   sqlite3.c     :  12461 us
  opt         sqlite3.c     :  46722 us
  codegen     sqlite3.c     : 104743 us
  link        sqlite3.so    :  38573 us

RCC -O2:
  preprocess  sqlite3.c     : 203788 us
  parse       sqlite3.c     :  54047 us
  typecheck   sqlite3.c     :  12113 us
  opt         sqlite3.c     :  48854 us
  codegen     sqlite3.c     : 132778 us
  link        sqlite3.so    :  40300 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       530 ms |    72% |
| RCC -O1   |       623 ms |     2% |
| RCC -O2   |       505 ms |    29% |
| TCC       |        87 ms |    25% |
| GCC -O0   |      1006 ms |     5% |
| GCC -O2   |     10563 ms |    15% |
| Clang -O0 |      1032 ms |    18% |
| Clang -O2 |      9026 ms |     4% |
