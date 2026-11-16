# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           74 |          774 |        848 |    17% |
| RCC -O1   |           44 |          887 |        931 |    18% |
| RCC -O2   |           49 |          454 |        503 |    42% |
| RCC -O3   |           48 |          412 |        460 |   100% |
| TCC       |           39 |          673 |        712 |   182% |
| GCC -O0   |          121 |          595 |        716 |    41% |
| GCC -O2   |          164 |          322 |        486 |    29% |
| Clang -O0 |           70 |          512 |        582 |    24% |
| Clang -O2 |          116 |          306 |        422 |    27% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          201 |         6082 |       6283 |    65% |
| RCC -O1   |          291 |         6693 |       6984 |    99% |
| RCC -O2   |          230 |         2537 |       2767 |   104% |
| RCC -O3   |          234 |         2363 |       2597 |    44% |
| TCC       |          109 |         4909 |       5018 |    35% |
| GCC -O0   |          563 |         3731 |       4294 |    16% |
| GCC -O2   |          876 |         2016 |       2892 |    11% |
| Clang -O0 |          497 |         3353 |       3850 |     6% |
| Clang -O2 |          820 |         1913 |       2733 |    21% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    791 us
  parse       bench.c       :    235 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    224 us
  link        bench_rcc     :  41476 us

RCC -O1:
  preprocess  bench.c       :    727 us
  parse       bench.c       :    206 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     60 us
  codegen     bench.c       :    485 us
  link        bench_o1      :  32298 us

RCC -O2:
  preprocess  bench.c       :    899 us
  parse       bench.c       :    214 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     57 us
  codegen     bench.c       :   3350 us
  link        bench_o2      :  82259 us

RCC -O3:
  preprocess  bench.c       :   1434 us
  parse       bench.c       :    622 us
  typecheck   bench.c       :     13 us
  opt         bench.c       :    183 us
  codegen     bench.c       :   2372 us
  link        bench_o3      :  36404 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 687505 us
  parse       sqlite3.c     : 402495 us
  typecheck   sqlite3.c     :  42385 us
  codegen     sqlite3.c     : 238585 us
  link        sqlite3.so    :  57398 us

RCC -O1:
  preprocess  sqlite3.c     : 442062 us
  parse       sqlite3.c     :  76269 us
  typecheck   sqlite3.c     :  71423 us
  opt         sqlite3.c     :  72677 us
  codegen     sqlite3.c     : 187482 us
  link        sqlite3.so    :  44593 us

RCC -O2:
  preprocess  sqlite3.c     : 353509 us
  parse       sqlite3.c     :  97626 us
  typecheck   sqlite3.c     :  13960 us
  opt         sqlite3.c     :  93781 us
  codegen     sqlite3.c     : 1608485 us
  link        sqlite3.so    :  54378 us

RCC -O3:
  preprocess  sqlite3.c     : 527907 us
  parse       sqlite3.c     : 132029 us
  typecheck   sqlite3.c     :  25341 us
  opt         sqlite3.c     :  96542 us
  codegen     sqlite3.c     : 1450440 us
  link        sqlite3.so    :  52339 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       479 ms |    59% |
| RCC -O1   |       477 ms |     1% |
| RCC -O2   |      1262 ms |     1% |
| RCC -O3   |      1197 ms |     7% |
| TCC       |        93 ms |    16% |
| GCC -O0   |      1068 ms |    14% |
| GCC -O2   |      9518 ms |     2% |
| Clang -O0 |       963 ms |    12% |
| Clang -O2 |      9477 ms |    13% |
