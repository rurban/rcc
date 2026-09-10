# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           61 |          713 |        774 |    88% |
| RCC -O1   |           72 |          692 |        764 |    36% |
| RCC -O2   |           69 |          691 |        760 |    54% |
| TCC       |           30 |          608 |        638 |   126% |
| GCC -O0   |           71 |          521 |        592 |   101% |
| GCC -O2   |          127 |          367 |        494 |    36% |
| Clang -O0 |          107 |          536 |        643 |    29% |
| Clang -O2 |           98 |          323 |        421 |    40% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          146 |         6415 |       6561 |    40% |
| RCC -O1   |          258 |         5694 |       5952 |    89% |
| RCC -O2   |          202 |         4857 |       5059 |   122% |
| TCC       |          127 |         5081 |       5208 |    46% |
| GCC -O0   |          570 |         3878 |       4448 |    22% |
| GCC -O2   |          962 |         2002 |       2964 |    15% |
| Clang -O0 |          530 |         3513 |       4043 |     8% |
| Clang -O2 |          877 |         1884 |       2761 |    22% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1015 us
  parse       bench.c       :    260 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    169 us
  link        bench_rcc     :    422 us
  link        bench_rcc     :  67252 us

RCC -O1:
  preprocess  bench.c       :    664 us
  parse       bench.c       :    180 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    123 us
  link        bench_o1      :    460 us
  link        bench_o1      :  59056 us

RCC -O2:
  preprocess  bench.c       :    721 us
  parse       bench.c       :    160 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    131 us
  link        bench_o2      :    636 us
  link        bench_o2      :  93849 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 253854 us
  parse       sqlite3.c     : 193059 us
  typecheck   sqlite3.c     :  25917 us
  codegen     sqlite3.c     : 171717 us
  link        sqlite3.so    :  16342 us

RCC -O1:
  preprocess  sqlite3.c     : 337168 us
  parse       sqlite3.c     :  55400 us
  typecheck   sqlite3.c     :  42865 us
  opt         sqlite3.c     :  34297 us
  codegen     sqlite3.c     : 154663 us
  link        sqlite3.so    :  16011 us

RCC -O2:
  preprocess  sqlite3.c     : 282309 us
  parse       sqlite3.c     :  58919 us
  typecheck   sqlite3.c     :  16316 us
  opt         sqlite3.c     :  29489 us
  codegen     sqlite3.c     : 148446 us
  link        sqlite3.so    :  26954 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       446 ms |    33% |
| RCC -O1   |       415 ms |     5% |
| RCC -O2   |       421 ms |     2% |
| TCC       |        88 ms |     7% |
| GCC -O0   |       980 ms |     1% |
| GCC -O2   |      9619 ms |     0% |
| Clang -O0 |       989 ms |     9% |
| Clang -O2 |      9893 ms |     2% |
