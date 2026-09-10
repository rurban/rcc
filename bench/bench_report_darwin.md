# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           52 |          666 |        718 |     7% |
| RCC -O1   |           56 |          662 |        718 |     7% |
| RCC -O2   |           58 |          655 |        713 |    13% |
| TCC       |           25 |          556 |        581 |    60% |
| GCC -O0   |           68 |          494 |        562 |    13% |
| GCC -O2   |          108 |          297 |        405 |    65% |
| Clang -O0 |           63 |          458 |        521 |    25% |
| Clang -O2 |           88 |          266 |        354 |     4% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          117 |         4412 |       4529 |    12% |
| RCC -O1   |          116 |         4350 |       4466 |     8% |
| RCC -O2   |          116 |         3925 |       4041 |    31% |
| TCC       |           98 |         3783 |       3881 |    27% |
| GCC -O0   |          462 |         3084 |       3546 |    19% |
| GCC -O2   |          794 |         1743 |       2537 |     5% |
| Clang -O0 |          452 |         3086 |       3538 |     5% |
| Clang -O2 |          782 |         1745 |       2527 |     6% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    572 us
  parse       bench.c       :    116 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    125 us
  link        bench_rcc     :    101 us
  link        bench_rcc     :  45983 us

RCC -O1:
  preprocess  bench.c       :    572 us
  parse       bench.c       :    118 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    111 us
  link        bench_o1      :     99 us
  link        bench_o1      :  45071 us

RCC -O2:
  preprocess  bench.c       :    525 us
  parse       bench.c       :    119 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    112 us
  link        bench_o2      :    145 us
  link        bench_o2      :  46840 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 189278 us
  parse       sqlite3.c     :  48218 us
  typecheck   sqlite3.c     :  12232 us
  codegen     sqlite3.c     : 137246 us
  link        sqlite3.so    :  15360 us

RCC -O1:
  preprocess  sqlite3.c     : 213993 us
  parse       sqlite3.c     :  60386 us
  typecheck   sqlite3.c     :  15136 us
  opt         sqlite3.c     :  26056 us
  codegen     sqlite3.c     :  89982 us
  link        sqlite3.so    :  13370 us

RCC -O2:
  preprocess  sqlite3.c     : 215727 us
  parse       sqlite3.c     :  45788 us
  typecheck   sqlite3.c     :  10266 us
  opt         sqlite3.c     :  24368 us
  codegen     sqlite3.c     : 128442 us
  link        sqlite3.so    :  15510 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       388 ms |    23% |
| RCC -O1   |       388 ms |     0% |
| RCC -O2   |       397 ms |     0% |
| TCC       |        83 ms |     9% |
| GCC -O0   |       921 ms |     1% |
| GCC -O2   |      8516 ms |     5% |
| Clang -O0 |       904 ms |     8% |
| Clang -O2 |      8568 ms |     0% |
