# Linux RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           27 |          762 |        789 |
| RCC -O1   |           26 |          782 |        808 |
| RCC -O2   |           26 |          764 |        790 |
| TCC       |           24 |          536 |        560 |
| SLIMCC    |          134 |          538 |        672 |
| XCC       |           45 |          396 |        441 |
| KEFIR     |          258 |          613 |        871 |
| KEFIR -O1 |          215 |          335 |        550 |
| SCC       |          124 |          681 |        805 |
| LACC      |           45 |          830 |        875 |
| ANTCC     |           31 |          454 |        485 |
| CAKE      |          269 |          515 |        784 |
| CCC       |           39 |          590 |        629 |
| GCC -O0   |           69 |          525 |        594 |
| GCC -O2   |          235 |          195 |        430 |
| Clang -O0 |          563 |          510 |       1073 |
| Clang -O2 |          220 |          196 |        416 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          356 |         6293 |       6649 |
| RCC -O1   |          337 |         7213 |       7550 |
| RCC -O2   |          343 |         6577 |       6920 |
| TCC       |           86 |         5113 |       5199 |
| SLIMCC    |          297 |         5019 |       5316 |
| KEFIR     |         4458 |         5379 |       9837 |
| KEFIR -O1 |         5122 |         3713 |       8835 |
| ANTCC     |          200 |         3827 |       4027 |
| CCC       |          888 |         4888 |       5776 |
| GCC -O0   |          928 |         4897 |       5825 |
| GCC -O2   |         2091 |         2616 |       4707 |
| Clang -O0 |          930 |         4965 |       5895 |
| Clang -O2 |         1694 |         2787 |       4481 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  19803 us
  parse       bench.c       :   1576 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :   3754 us
  link        bench_rcc     :  12081 us

RCC -O1:
  preprocess  bench.c       :   9839 us
  parse       bench.c       :    589 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    434 us
  link        bench_o1      :   9954 us

RCC -O2:
  preprocess  bench.c       :  10878 us
  parse       bench.c       :    612 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     39 us
  codegen     bench.c       :    304 us
  link        bench_o2      :   9636 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 298283 us
  parse       sqlite3.c     : 151532 us
  typecheck   sqlite3.c     :  10478 us
  codegen     sqlite3.c     : 219881 us
  link        sqlite3.so    :   9477 us

RCC -O1:
  preprocess  sqlite3.c     : 257384 us
  parse       sqlite3.c     : 149373 us
  typecheck   sqlite3.c     :  10404 us
  opt         sqlite3.c     : 237354 us
  codegen     sqlite3.c     : 211387 us
  link        sqlite3.so    :   9226 us

RCC -O2:
  preprocess  sqlite3.c     : 259022 us
  parse       sqlite3.c     : 149672 us
  typecheck   sqlite3.c     :  10472 us
  opt         sqlite3.c     : 250742 us
  codegen     sqlite3.c     : 213163 us
  link        sqlite3.so    :   9741 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1034 ms |
| RCC -O1   |      1191 ms |
| RCC -O2   |      1206 ms |
| TCC       |       125 ms |
| SLIMCC    |       671 ms |
| KEFIR     |     19782 ms |
| KEFIR -O1 |     35169 ms |
| ANTCC     |       446 ms |
| CCC       |     13717 ms |
| GCC -O0   |      5250 ms |
| GCC -O2   |     28705 ms |
| Clang -O0 |      1894 ms |
| Clang -O2 |     21475 ms |
