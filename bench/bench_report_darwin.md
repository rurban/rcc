# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          229 |          808 |       1037 |
| RCC -O1   |          203 |          824 |       1027 |
| RCC -O2   |           86 |          710 |        796 |
| TCC       |           72 |          619 |        691 |
| GCC -O0   |           92 |          548 |        640 |
| GCC -O2   |          133 |          325 |        458 |
| Clang -O0 |          139 |          597 |        736 |
| Clang -O2 |          115 |          339 |        454 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          281 |         6722 |       7003 |
| RCC -O1   |          342 |         6286 |       6628 |
| RCC -O2   |          288 |         5000 |       5288 |
| TCC       |          198 |         5192 |       5390 |
| GCC -O0   |          946 |         3986 |       4932 |
| GCC -O2   |         1037 |         2141 |       3178 |
| Clang -O0 |          612 |         3535 |       4147 |
| Clang -O2 |         1022 |         1950 |       2972 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    728 us
  parse       bench.c       :    157 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    128 us
  link        bench_rcc     :    227 us
  link        bench_rcc     :  61052 us

RCC -O1:
  preprocess  bench.c       :   1424 us
  parse       bench.c       :    156 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     31 us
  codegen     bench.c       :    176 us
  link        bench_o1      :    355 us
  link        bench_o1      :  60319 us

RCC -O2:
  preprocess  bench.c       :   1125 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    148 us
  link        bench_o2      :    240 us
  link        bench_o2      :  70569 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 449913 us
  parse       sqlite3.c     : 269473 us
  typecheck   sqlite3.c     :  31716 us
  codegen     sqlite3.c     : 376257 us
  link        sqlite3.so    :  37279 us

RCC -O1:
  preprocess  sqlite3.c     : 524677 us
  parse       sqlite3.c     : 154638 us
  typecheck   sqlite3.c     :  25002 us
  opt         sqlite3.c     : 319801 us
  codegen     sqlite3.c     : 209701 us
  link        sqlite3.so    :  21472 us

RCC -O2:
  preprocess  sqlite3.c     : 442519 us
  parse       sqlite3.c     : 176301 us
  typecheck   sqlite3.c     :  31667 us
  opt         sqlite3.c     : 451794 us
  codegen     sqlite3.c     : 213057 us
  link        sqlite3.so    :  28251 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1205 ms |
| RCC -O1   |       716 ms |
| RCC -O2   |       702 ms |
| TCC       |       116 ms |
| GCC -O0   |      1010 ms |
| GCC -O2   |     12782 ms |
| Clang -O0 |      1396 ms |
| Clang -O2 |     10059 ms |
