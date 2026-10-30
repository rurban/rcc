# Darwin RCC Benchmark Results

_Generated: October 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           28 |          703 |        731 |    25% |
| RCC -O1   |           31 |          690 |        721 |    31% |
| RCC -O2   |           32 |          672 |        704 |    31% |
| TCC       |           26 |          554 |        580 |    88% |
| GCC -O0   |           60 |          486 |        546 |    48% |
| GCC -O2   |           89 |          292 |        381 |    46% |
| Clang -O0 |           58 |          487 |        545 |     5% |
| Clang -O2 |           95 |          296 |        391 |    22% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          138 |         5344 |       5482 |   110% |
| RCC -O1   |          148 |         4978 |       5126 |    32% |
| RCC -O2   |          160 |         4267 |       4427 |    50% |
| TCC       |          110 |         3750 |       3860 |   102% |
| GCC -O0   |          486 |         3248 |       3734 |    77% |
| GCC -O2   |          802 |         1766 |       2568 |     6% |
| Clang -O0 |          453 |         3157 |       3610 |     6% |
| Clang -O2 |          782 |         1744 |       2526 |     9% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    681 us
  parse       bench.c       :    177 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    163 us
  link        bench_rcc     :  27114 us

RCC -O1:
  preprocess  bench.c       :    533 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     46 us
  codegen     bench.c       :    128 us
  link        bench_o1      :  22226 us

RCC -O2:
  preprocess  bench.c       :    585 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     52 us
  codegen     bench.c       :    132 us
  link        bench_o2      :  23309 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 232865 us
  parse       sqlite3.c     :  93028 us
  typecheck   sqlite3.c     :  22588 us
  codegen     sqlite3.c     : 147301 us
  link        sqlite3.so    :  46968 us

RCC -O1:
  preprocess  sqlite3.c     : 242019 us
  parse       sqlite3.c     :  59593 us
  typecheck   sqlite3.c     :  13046 us
  opt         sqlite3.c     :  52675 us
  codegen     sqlite3.c     : 112856 us
  link        sqlite3.so    :  30739 us

RCC -O2:
  preprocess  sqlite3.c     : 230211 us
  parse       sqlite3.c     :  64430 us
  typecheck   sqlite3.c     :  18607 us
  opt         sqlite3.c     :  78838 us
  codegen     sqlite3.c     : 134645 us
  link        sqlite3.so    :  30198 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       439 ms |    32% |
| RCC -O1   |       429 ms |     7% |
| RCC -O2   |       436 ms |     1% |
| TCC       |        85 ms |    12% |
| GCC -O0   |       929 ms |     0% |
| GCC -O2   |      8699 ms |     4% |
| Clang -O0 |       911 ms |     4% |
| Clang -O2 |      8595 ms |     0% |
