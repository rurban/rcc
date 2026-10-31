# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           23 |          581 |        604 |     5% |
| RCC -O1   |           28 |          603 |        631 |     3% |
| RCC -O2   |           25 |          556 |        581 |     4% |
| TCC       |           24 |          469 |        493 |    83% |
| GCC -O0   |           56 |          399 |        455 |     8% |
| GCC -O2   |           76 |          242 |        318 |    15% |
| Clang -O0 |           46 |          398 |        444 |    15% |
| Clang -O2 |           73 |          239 |        312 |     7% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          117 |         3916 |       4033 |     1% |
| RCC -O1   |          119 |         3870 |       3989 |     5% |
| RCC -O2   |          117 |         3497 |       3614 |     4% |
| TCC       |           88 |         3329 |       3417 |     6% |
| GCC -O0   |          378 |         2758 |       3136 |     2% |
| GCC -O2   |          680 |         1579 |       2259 |     9% |
| Clang -O0 |          374 |         2812 |       3186 |     3% |
| Clang -O2 |          662 |         1589 |       2251 |     3% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    511 us
  parse       bench.c       :    121 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    117 us
  link        bench_rcc     :  19632 us

RCC -O1:
  preprocess  bench.c       :    649 us
  parse       bench.c       :    114 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     46 us
  codegen     bench.c       :    113 us
  link        bench_o1      :  18622 us

RCC -O2:
  preprocess  bench.c       :    460 us
  parse       bench.c       :    133 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     45 us
  codegen     bench.c       :    115 us
  link        bench_o2      :  18326 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 159017 us
  parse       sqlite3.c     :  42491 us
  typecheck   sqlite3.c     :  10464 us
  codegen     sqlite3.c     :  88487 us
  link        sqlite3.so    :  24668 us

RCC -O1:
  preprocess  sqlite3.c     : 159056 us
  parse       sqlite3.c     :  40635 us
  typecheck   sqlite3.c     :  10823 us
  opt         sqlite3.c     :  36020 us
  codegen     sqlite3.c     :  89860 us
  link        sqlite3.so    :  25256 us

RCC -O2:
  preprocess  sqlite3.c     : 163396 us
  parse       sqlite3.c     :  43004 us
  typecheck   sqlite3.c     :  10947 us
  opt         sqlite3.c     :  39285 us
  codegen     sqlite3.c     :  93380 us
  link        sqlite3.so    :  24677 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       400 ms |    13% |
| RCC -O1   |       400 ms |    14% |
| RCC -O2   |       394 ms |     0% |
| TCC       |        74 ms |     1% |
| GCC -O0   |       796 ms |     0% |
| GCC -O2   |      7712 ms |     1% |
| Clang -O0 |       796 ms |     0% |
| Clang -O2 |      7639 ms |     0% |
