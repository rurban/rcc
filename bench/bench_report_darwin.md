# Darwin RCC Benchmark Results

_Generated: October 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           28 |          636 |        664 |   100% |
| RCC -O1   |           33 |          661 |        694 |    45% |
| RCC -O2   |           32 |          661 |        693 |     3% |
| TCC       |           26 |          549 |        575 |    80% |
| GCC -O0   |           63 |          461 |        524 |    39% |
| GCC -O2   |           89 |          282 |        371 |    23% |
| Clang -O0 |           56 |          469 |        525 |    25% |
| Clang -O2 |           88 |          285 |        373 |     9% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          146 |         4978 |       5124 |     8% |
| RCC -O1   |          139 |         5275 |       5414 |    31% |
| RCC -O2   |          138 |         4747 |       4885 |    60% |
| TCC       |          100 |         4309 |       4409 |    22% |
| GCC -O0   |          481 |         3176 |       3657 |    35% |
| GCC -O2   |          895 |         1968 |       2863 |    46% |
| Clang -O0 |          459 |         3329 |       3788 |    18% |
| Clang -O2 |          798 |         2024 |       2822 |    13% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    650 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    153 us
  link        bench_rcc     :  28100 us

RCC -O1:
  preprocess  bench.c       :    617 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     51 us
  codegen     bench.c       :    155 us
  link        bench_o1      :  22818 us

RCC -O2:
  preprocess  bench.c       :    514 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     46 us
  codegen     bench.c       :    131 us
  link        bench_o2      :  22620 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 250355 us
  parse       sqlite3.c     : 141519 us
  typecheck   sqlite3.c     :  15862 us
  codegen     sqlite3.c     : 114215 us
  link        sqlite3.so    :  33527 us

RCC -O1:
  preprocess  sqlite3.c     : 227837 us
  parse       sqlite3.c     :  55162 us
  typecheck   sqlite3.c     :  15303 us
  opt         sqlite3.c     :  44916 us
  codegen     sqlite3.c     : 142605 us
  link        sqlite3.so    :  34882 us

RCC -O2:
  preprocess  sqlite3.c     : 241680 us
  parse       sqlite3.c     :  52826 us
  typecheck   sqlite3.c     :  11897 us
  opt         sqlite3.c     :  86654 us
  codegen     sqlite3.c     : 144390 us
  link        sqlite3.so    :  30168 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       486 ms |    56% |
| RCC -O1   |       477 ms |     4% |
| RCC -O2   |       486 ms |     1% |
| TCC       |        85 ms |    31% |
| GCC -O0   |      1024 ms |     0% |
| GCC -O2   |      8830 ms |     3% |
| Clang -O0 |       975 ms |     9% |
| Clang -O2 |      8750 ms |     2% |
