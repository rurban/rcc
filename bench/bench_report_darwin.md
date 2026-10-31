# Darwin RCC Benchmark Results

_Generated: October 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           38 |          744 |        782 |   113% |
| RCC -O1   |           34 |          681 |        715 |    47% |
| RCC -O2   |           33 |          691 |        724 |    18% |
| TCC       |           32 |          672 |        704 |    96% |
| GCC -O0   |           75 |          593 |        668 |   122% |
| GCC -O2   |          169 |          353 |        522 |    85% |
| Clang -O0 |           85 |          596 |        681 |    21% |
| Clang -O2 |          139 |          352 |        491 |    79% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          325 |         7248 |       7573 |    44% |
| RCC -O1   |          228 |         6484 |       6712 |    74% |
| RCC -O2   |          262 |         5891 |       6153 |   104% |
| TCC       |          152 |         5577 |       5729 |   172% |
| GCC -O0   |          599 |         4078 |       4677 |    12% |
| GCC -O2   |          917 |         2236 |       3153 |    40% |
| Clang -O0 |          517 |         3704 |       4221 |    13% |
| Clang -O2 |         1048 |         2106 |       3154 |     9% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    822 us
  parse       bench.c       :    230 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    163 us
  link        bench_rcc     :  28725 us

RCC -O1:
  preprocess  bench.c       :    590 us
  parse       bench.c       :    140 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     50 us
  codegen     bench.c       :    146 us
  link        bench_o1      : 106880 us

RCC -O2:
  preprocess  bench.c       :   1145 us
  parse       bench.c       :    219 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     60 us
  codegen     bench.c       :    251 us
  link        bench_o2      :  32610 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 265327 us
  parse       sqlite3.c     : 348698 us
  typecheck   sqlite3.c     :  27528 us
  codegen     sqlite3.c     : 166298 us
  link        sqlite3.so    :  75037 us

RCC -O1:
  preprocess  sqlite3.c     : 365094 us
  parse       sqlite3.c     :  85706 us
  typecheck   sqlite3.c     :  25041 us
  opt         sqlite3.c     :  60312 us
  codegen     sqlite3.c     : 189605 us
  link        sqlite3.so    :  48326 us

RCC -O2:
  preprocess  sqlite3.c     : 286409 us
  parse       sqlite3.c     :  73226 us
  typecheck   sqlite3.c     :  14651 us
  opt         sqlite3.c     :  62481 us
  codegen     sqlite3.c     : 167180 us
  link        sqlite3.so    :  37054 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       529 ms |    25% |
| RCC -O1   |       465 ms |    10% |
| RCC -O2   |       602 ms |     6% |
| TCC       |       100 ms |    13% |
| GCC -O0   |      1111 ms |     2% |
| GCC -O2   |     11335 ms |    23% |
| Clang -O0 |      1169 ms |    11% |
| Clang -O2 |     11971 ms |     1% |
