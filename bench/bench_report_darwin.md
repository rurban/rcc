# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           84 |          779 |        863 |    14% |
| RCC -O1   |           91 |          762 |        853 |    32% |
| RCC -O2   |           91 |          760 |        851 |    11% |
| TCC       |           35 |          571 |        606 |   148% |
| GCC -O0   |           75 |          460 |        535 |    61% |
| GCC -O2   |           95 |          288 |        383 |    54% |
| Clang -O0 |           70 |          505 |        575 |    90% |
| Clang -O2 |          124 |          345 |        469 |    50% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          210 |         5340 |       5550 |    68% |
| RCC -O1   |          180 |         4923 |       5103 |    39% |
| RCC -O2   |          136 |         4718 |       4854 |    67% |
| TCC       |          232 |         4854 |       5086 |    64% |
| GCC -O0   |          498 |         3343 |       3841 |    24% |
| GCC -O2   |          904 |         2246 |       3150 |    20% |
| Clang -O0 |          572 |         4130 |       4702 |    29% |
| Clang -O2 |          927 |         2089 |       3016 |    18% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1768 us
  parse       bench.c       :    390 us
  typecheck   bench.c       :      8 us
  codegen     bench.c       :    251 us
  link        bench_rcc     :    308 us
  link        bench_rcc     :  79856 us

RCC -O1:
  preprocess  bench.c       :    700 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    166 us
  link        bench_o1      :    395 us
  link        bench_o1      :  86237 us

RCC -O2:
  preprocess  bench.c       :    701 us
  parse       bench.c       :    172 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     30 us
  codegen     bench.c       :    138 us
  link        bench_o2      :    239 us
  link        bench_o2      :  68320 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 288895 us
  parse       sqlite3.c     :  81887 us
  typecheck   sqlite3.c     :  18630 us
  codegen     sqlite3.c     : 170783 us
  link        sqlite3.so    :  22595 us

RCC -O1:
  preprocess  sqlite3.c     : 278731 us
  parse       sqlite3.c     :  66393 us
  typecheck   sqlite3.c     :  13659 us
  opt         sqlite3.c     :  28071 us
  codegen     sqlite3.c     : 176887 us
  link        sqlite3.so    :  17920 us

RCC -O2:
  preprocess  sqlite3.c     : 280521 us
  parse       sqlite3.c     :  80728 us
  typecheck   sqlite3.c     :  15886 us
  opt         sqlite3.c     :  47834 us
  codegen     sqlite3.c     : 160831 us
  link        sqlite3.so    :  22338 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       486 ms |    33% |
| RCC -O1   |       416 ms |     2% |
| RCC -O2   |       468 ms |     0% |
| TCC       |       116 ms |     6% |
| GCC -O0   |      1025 ms |     8% |
| GCC -O2   |      9671 ms |     5% |
| Clang -O0 |       971 ms |     1% |
| Clang -O2 |      9297 ms |     0% |
