# Darwin RCC Benchmark Results

_Generated: October 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           67 |         1020 |       1087 |    38% |
| RCC -O1   |          145 |          910 |       1055 |   176% |
| RCC -O2   |           38 |          861 |        899 |    34% |
| TCC       |           33 |          582 |        615 |   139% |
| GCC -O0   |           79 |          619 |        698 |    54% |
| GCC -O2   |          150 |          317 |        467 |    43% |
| Clang -O0 |           78 |          568 |        646 |    25% |
| Clang -O2 |          103 |          346 |        449 |    47% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          189 |         5600 |       5789 |    77% |
| RCC -O1   |          240 |         5274 |       5514 |    71% |
| RCC -O2   |          161 |         4571 |       4732 |   149% |
| TCC       |          108 |         4119 |       4227 |    61% |
| GCC -O0   |          482 |         3371 |       3853 |    15% |
| GCC -O2   |          870 |         1815 |       2685 |    24% |
| Clang -O0 |          453 |         3160 |       3613 |     1% |
| Clang -O2 |          790 |         1795 |       2585 |    10% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2186 us
  parse       bench.c       :    473 us
  typecheck   bench.c       :      9 us
  codegen     bench.c       :    522 us
  link        bench_rcc     :  56929 us

RCC -O1:
  preprocess  bench.c       :   1779 us
  parse       bench.c       :    378 us
  typecheck   bench.c       :      8 us
  opt         bench.c       :     96 us
  codegen     bench.c       :    344 us
  link        bench_o1      :  66172 us

RCC -O2:
  preprocess  bench.c       :   1083 us
  parse       bench.c       :    214 us
  typecheck   bench.c       :     34 us
  opt         bench.c       :     59 us
  codegen     bench.c       :    174 us
  link        bench_o2      :  69563 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 862790 us
  parse       sqlite3.c     : 339693 us
  typecheck   sqlite3.c     :  44351 us
  codegen     sqlite3.c     : 388290 us
  link        sqlite3.so    :  82294 us

RCC -O1:
  preprocess  sqlite3.c     : 501849 us
  parse       sqlite3.c     : 111315 us
  typecheck   sqlite3.c     :  39679 us
  opt         sqlite3.c     :  78498 us
  codegen     sqlite3.c     : 258525 us
  link        sqlite3.so    :  77674 us

RCC -O2:
  preprocess  sqlite3.c     : 541265 us
  parse       sqlite3.c     : 115399 us
  typecheck   sqlite3.c     :  43180 us
  opt         sqlite3.c     : 131689 us
  codegen     sqlite3.c     : 348799 us
  link        sqlite3.so    :  78181 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       475 ms |    72% |
| RCC -O1   |       431 ms |     8% |
| RCC -O2   |       434 ms |     9% |
| TCC       |        85 ms |    12% |
| GCC -O0   |       940 ms |     0% |
| GCC -O2   |      8984 ms |     6% |
| Clang -O0 |       908 ms |     0% |
| Clang -O2 |      8820 ms |     3% |
