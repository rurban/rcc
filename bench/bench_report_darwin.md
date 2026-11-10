# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           50 |          947 |        997 |    32% |
| RCC -O1   |           76 |          912 |        988 |    85% |
| RCC -O2   |           56 |          467 |        523 |    37% |
| RCC -O3   |           60 |          448 |        508 |    35% |
| TCC       |           37 |          792 |        829 |   189% |
| GCC -O0   |          115 |          691 |        806 |   132% |
| GCC -O2   |          206 |          376 |        582 |    26% |
| Clang -O0 |          125 |          711 |        836 |    41% |
| Clang -O2 |          183 |          431 |        614 |    41% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          212 |         7649 |       7861 |    80% |
| RCC -O1   |          243 |         6887 |       7130 |    22% |
| RCC -O2   |          279 |         2504 |       2783 |    70% |
| RCC -O3   |          254 |         3895 |       4149 |    33% |
| TCC       |          200 |         5992 |       6192 |    59% |
| GCC -O0   |          661 |         4592 |       5253 |    49% |
| GCC -O2   |         1274 |         2553 |       3827 |    55% |
| Clang -O0 |          663 |         4537 |       5200 |    41% |
| Clang -O2 |         1311 |         2547 |       3858 |    10% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1018 us
  parse       bench.c       :    319 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    332 us
  link        bench_rcc     :  60407 us

RCC -O1:
  preprocess  bench.c       :    730 us
  parse       bench.c       :    180 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     84 us
  codegen     bench.c       :    248 us
  link        bench_o1      :  47284 us

RCC -O2:
  preprocess  bench.c       :    814 us
  parse       bench.c       :    169 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    233 us
  codegen     bench.c       :   3147 us
  link        bench_o2      :  45793 us

RCC -O3:
  preprocess  bench.c       :    777 us
  parse       bench.c       :    181 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     56 us
  codegen     bench.c       :   2185 us
  link        bench_o3      :  41428 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 490868 us
  parse       sqlite3.c     : 294766 us
  typecheck   sqlite3.c     :  25493 us
  codegen     sqlite3.c     : 262055 us
  link        sqlite3.so    :  73883 us

RCC -O1:
  preprocess  sqlite3.c     : 382155 us
  parse       sqlite3.c     : 106684 us
  typecheck   sqlite3.c     :  30045 us
  opt         sqlite3.c     :  81569 us
  codegen     sqlite3.c     : 227670 us
  link        sqlite3.so    :  99873 us

RCC -O2:
  preprocess  sqlite3.c     : 493904 us
  parse       sqlite3.c     : 108583 us
  typecheck   sqlite3.c     :  32288 us
  opt         sqlite3.c     : 115998 us
  codegen     sqlite3.c     : 1764863 us
  link        sqlite3.so    :  80643 us

RCC -O3:
  preprocess  sqlite3.c     : 447484 us
  parse       sqlite3.c     : 119793 us
  typecheck   sqlite3.c     :  21391 us
  opt         sqlite3.c     :  84379 us
  codegen     sqlite3.c     : 1656713 us
  link        sqlite3.so    :  60006 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       695 ms |    80% |
| RCC -O1   |       672 ms |     1% |
| RCC -O2   |      1726 ms |     6% |
| RCC -O3   |      1511 ms |    19% |
| TCC       |       176 ms |    27% |
| GCC -O0   |      1245 ms |    32% |
| GCC -O2   |     13272 ms |     3% |
| Clang -O0 |      1450 ms |     4% |
| Clang -O2 |     13047 ms |     7% |
