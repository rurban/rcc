# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           68 |          856 |        924 |    16% |
| RCC -O1   |           40 |          973 |       1013 |    45% |
| RCC -O2   |           82 |          736 |        818 |   345% |
| TCC       |           35 |          623 |        658 |    77% |
| GCC -O0   |           97 |          630 |        727 |    31% |
| GCC -O2   |          181 |          371 |        552 |    24% |
| Clang -O0 |           97 |          663 |        760 |    93% |
| Clang -O2 |          187 |          413 |        600 |    25% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          379 |         6660 |       7039 |    61% |
| RCC -O1   |          204 |         6623 |       6827 |    55% |
| RCC -O2   |          169 |         6083 |       6252 |   125% |
| TCC       |          226 |         5701 |       5927 |   126% |
| GCC -O0   |          637 |         3670 |       4307 |    15% |
| GCC -O2   |         1021 |         2365 |       3386 |     9% |
| Clang -O0 |          606 |         3994 |       4600 |    19% |
| Clang -O2 |         1030 |         2266 |       3296 |    10% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1070 us
  parse       bench.c       :    197 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    186 us
  link        bench_rcc     :  48737 us

RCC -O1:
  preprocess  bench.c       :    720 us
  parse       bench.c       :    153 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     47 us
  codegen     bench.c       :    174 us
  link        bench_o1      :  47311 us

RCC -O2:
  preprocess  bench.c       :   1519 us
  parse       bench.c       :    240 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :    152 us
  codegen     bench.c       :    440 us
  link        bench_o2      :  33676 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 917135 us
  parse       sqlite3.c     : 425424 us
  typecheck   sqlite3.c     :  20455 us
  codegen     sqlite3.c     : 262570 us
  link        sqlite3.so    :  80489 us

RCC -O1:
  preprocess  sqlite3.c     : 472274 us
  parse       sqlite3.c     :  89851 us
  typecheck   sqlite3.c     :  19201 us
  opt         sqlite3.c     :  55496 us
  codegen     sqlite3.c     : 304879 us
  link        sqlite3.so    :  66863 us

RCC -O2:
  preprocess  sqlite3.c     : 361137 us
  parse       sqlite3.c     : 116193 us
  typecheck   sqlite3.c     :  31590 us
  opt         sqlite3.c     : 177793 us
  codegen     sqlite3.c     : 442104 us
  link        sqlite3.so    :  80530 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       580 ms |    62% |
| RCC -O1   |       545 ms |     8% |
| RCC -O2   |       616 ms |    19% |
| TCC       |       129 ms |     7% |
| GCC -O0   |      1299 ms |    16% |
| GCC -O2   |     10020 ms |    27% |
| Clang -O0 |      1045 ms |     0% |
| Clang -O2 |     12777 ms |    39% |
