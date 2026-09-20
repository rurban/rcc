# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           40 |          693 |        733 |    27% |
| RCC -O1   |           32 |          692 |        724 |    34% |
| RCC -O2   |           35 |          743 |        778 |    25% |
| TCC       |           34 |          557 |        591 |    76% |
| GCC -O0   |           84 |          469 |        553 |    39% |
| GCC -O2   |           97 |          286 |        383 |     9% |
| Clang -O0 |           66 |          466 |        532 |    15% |
| Clang -O2 |           97 |          282 |        379 |     8% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          146 |         5582 |       5728 |    13% |
| RCC -O1   |          198 |         4844 |       5042 |    61% |
| RCC -O2   |          148 |         4369 |       4517 |    49% |
| TCC       |          116 |         4270 |       4386 |    51% |
| GCC -O0   |          509 |         3396 |       3905 |    11% |
| GCC -O2   |          883 |         1970 |       2853 |     6% |
| Clang -O0 |          491 |         3381 |       3872 |     9% |
| Clang -O2 |          855 |         1934 |       2789 |     7% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1110 us
  parse       bench.c       :    190 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    176 us
  link        bench_rcc     :  30917 us

RCC -O1:
  preprocess  bench.c       :    678 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     52 us
  codegen     bench.c       :    161 us
  link        bench_o1      :  28291 us

RCC -O2:
  preprocess  bench.c       :    589 us
  parse       bench.c       :    140 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     53 us
  codegen     bench.c       :    151 us
  link        bench_o2      :  26582 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 279064 us
  parse       sqlite3.c     : 196113 us
  typecheck   sqlite3.c     :  24945 us
  codegen     sqlite3.c     : 171839 us
  link        sqlite3.so    :  45066 us

RCC -O1:
  preprocess  sqlite3.c     : 348154 us
  parse       sqlite3.c     :  90986 us
  typecheck   sqlite3.c     :  17869 us
  opt         sqlite3.c     :  45138 us
  codegen     sqlite3.c     : 126360 us
  link        sqlite3.so    :  38902 us

RCC -O2:
  preprocess  sqlite3.c     : 333959 us
  parse       sqlite3.c     :  94294 us
  typecheck   sqlite3.c     :  26346 us
  opt         sqlite3.c     :  86635 us
  codegen     sqlite3.c     : 247341 us
  link        sqlite3.so    :  85916 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       504 ms |    73% |
| RCC -O1   |       475 ms |     0% |
| RCC -O2   |       479 ms |     0% |
| TCC       |        92 ms |    14% |
| GCC -O0   |      1027 ms |    12% |
| GCC -O2   |      9575 ms |     3% |
| Clang -O0 |       995 ms |     7% |
| Clang -O2 |      9573 ms |     2% |
