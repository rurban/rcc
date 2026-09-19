# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           14 |          703 |        717 |     4% |
| RCC -O1   |           14 |          701 |        715 |     7% |
| RCC -O2   |           14 |          700 |        714 |     0% |
| TCC       |            5 |          486 |        491 |     0% |
| SLIMCC    |           31 |          498 |        529 |     3% |
| XCC       |            8 |          358 |        366 |   262% |
| KEFIR     |          176 |          570 |        746 |     9% |
| KEFIR -O1 |          184 |          307 |        491 |     1% |
| SCC       |           32 |          536 |        568 |   546% |
| LACC      |           21 |          756 |        777 |     0% |
| ANTCC     |           23 |          413 |        436 |     4% |
| CAKE      |           88 |          477 |        565 |     1% |
| BCC       |           27 |          556 |        583 |    29% |
| CCC       |           32 |          535 |        567 |    14% |
| GCC -O0   |           54 |          478 |        532 |     1% |
| GCC -O2   |          144 |          175 |        319 |    25% |
| Clang -O0 |           77 |          463 |        540 |  1042% |
| Clang -O2 |          124 |          176 |        300 |    90% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          272 |         5801 |       6073 |    11% |
| RCC -O1   |          274 |         6546 |       6820 |     0% |
| RCC -O2   |          273 |         6058 |       6331 |     1% |
| TCC       |           65 |         5047 |       5112 |     3% |
| SLIMCC    |          249 |         4911 |       5160 |     1% |
| KEFIR     |         4369 |         5293 |       9662 |     0% |
| KEFIR -O1 |         4821 |         3681 |       8502 |     0% |
| ANTCC     |          184 |         3721 |       3905 |     2% |
| CCC       |          855 |         4445 |       5300 |     1% |
| BCC       |         2966 |         4685 |       7651 |     1% |
| GCC -O0   |          837 |         4804 |       5641 |     1% |
| GCC -O2   |         1838 |         2517 |       4355 |     1% |
| Clang -O0 |          854 |         4561 |       5415 |     4% |
| Clang -O2 |         1573 |         2547 |       4120 |     3% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  11551 us
  parse       bench.c       :    582 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    304 us
  link        bench_rcc     :    888 us

RCC -O1:
  preprocess  bench.c       :  10422 us
  parse       bench.c       :    575 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     91 us
  codegen     bench.c       :    314 us
  link        bench_o1      :    871 us

RCC -O2:
  preprocess  bench.c       :  10327 us
  parse       bench.c       :    635 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     93 us
  codegen     bench.c       :    311 us
  link        bench_o2      :    805 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 259298 us
  parse       sqlite3.c     : 151046 us
  typecheck   sqlite3.c     :   7910 us
  codegen     sqlite3.c     : 208422 us
  link        sqlite3.so    :   8688 us

RCC -O1:
  preprocess  sqlite3.c     : 248317 us
  parse       sqlite3.c     : 148030 us
  typecheck   sqlite3.c     :   7943 us
  opt         sqlite3.c     :  64699 us
  codegen     sqlite3.c     : 206967 us
  link        sqlite3.so    :   9362 us

RCC -O2:
  preprocess  sqlite3.c     : 248228 us
  parse       sqlite3.c     : 147951 us
  typecheck   sqlite3.c     :   8025 us
  opt         sqlite3.c     :  73544 us
  codegen     sqlite3.c     : 207750 us
  link        sqlite3.so    :   9017 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       723 ms |     0% |
| RCC -O1   |       754 ms |     0% |
| RCC -O2   |       774 ms |     0% |
| TCC       |        97 ms |     0% |
| SLIMCC    |       642 ms |     0% |
| KEFIR     |     18262 ms |     0% |
| KEFIR -O1 |     32980 ms |     0% |
| ANTCC     |       389 ms |     0% |
| CCC       |     12571 ms |     0% |
| GCC -O0   |      4015 ms |     1% |
| GCC -O2   |     25531 ms |     0% |
| Clang -O0 |      1780 ms |     1% |
| Clang -O2 |     19850 ms |     0% |
