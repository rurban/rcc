# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           14 |          699 |        713 |     7% |
| RCC -O1   |           14 |          699 |        713 |     7% |
| RCC -O2   |           14 |          700 |        714 |     7% |
| TCC       |            5 |          483 |        488 |     0% |
| SLIMCC    |           31 |          496 |        527 |     3% |
| XCC       |            8 |          356 |        364 |    12% |
| KEFIR     |          176 |          569 |        745 |     1% |
| KEFIR -O1 |          184 |          306 |        490 |     2% |
| SCC       |           32 |          609 |        641 |     1% |
| LACC      |           22 |          756 |        778 |     0% |
| ANTCC     |           23 |          412 |        435 |     0% |
| CAKE      |           88 |          479 |        567 |     1% |
| BCC       |           27 |          558 |        585 |     0% |
| CCC       |           32 |          537 |        569 |    16% |
| GCC -O0   |           54 |          479 |        533 |     1% |
| GCC -O2   |          144 |          175 |        319 |     1% |
| Clang -O0 |           77 |          462 |        539 |     1% |
| Clang -O2 |          126 |          176 |        302 |     3% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          272 |         5815 |       6087 |     2% |
| RCC -O1   |          275 |         6549 |       6824 |     0% |
| RCC -O2   |          274 |         6066 |       6340 |     1% |
| TCC       |           66 |         5058 |       5124 |     1% |
| SLIMCC    |          249 |         4909 |       5158 |     0% |
| KEFIR     |         4373 |         5279 |       9652 |     0% |
| KEFIR -O1 |         4814 |         3691 |       8505 |     0% |
| ANTCC     |          187 |         3707 |       3894 |     1% |
| CCC       |          853 |         4468 |       5321 |     1% |
| BCC       |         2965 |         4679 |       7644 |     3% |
| GCC -O0   |          830 |         4804 |       5634 |     0% |
| GCC -O2   |         1845 |         2518 |       4363 |     0% |
| Clang -O0 |          866 |         4587 |       5453 |     0% |
| Clang -O2 |         1580 |         2544 |       4124 |     0% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  10113 us
  parse       bench.c       :    679 us
  typecheck   bench.c       :      7 us
  codegen     bench.c       :    375 us
  link        bench_rcc     :    896 us

RCC -O1:
  preprocess  bench.c       :  10257 us
  parse       bench.c       :    650 us
  typecheck   bench.c       :      7 us
  opt         bench.c       :     91 us
  codegen     bench.c       :    308 us
  link        bench_o1      :    892 us

RCC -O2:
  preprocess  bench.c       :  10203 us
  parse       bench.c       :    681 us
  typecheck   bench.c       :      7 us
  opt         bench.c       :     85 us
  codegen     bench.c       :    421 us
  link        bench_o2      :    863 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 250876 us
  parse       sqlite3.c     : 152023 us
  typecheck   sqlite3.c     :   8363 us
  codegen     sqlite3.c     : 202092 us
  link        sqlite3.so    :   8925 us

RCC -O1:
  preprocess  sqlite3.c     : 249871 us
  parse       sqlite3.c     : 150609 us
  typecheck   sqlite3.c     :   8204 us
  opt         sqlite3.c     :  64086 us
  codegen     sqlite3.c     : 201005 us
  link        sqlite3.so    :  16599 us

RCC -O2:
  preprocess  sqlite3.c     : 248456 us
  parse       sqlite3.c     : 150397 us
  typecheck   sqlite3.c     :   8201 us
  opt         sqlite3.c     :  74423 us
  codegen     sqlite3.c     : 202714 us
  link        sqlite3.so    :  16409 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       724 ms |     0% |
| RCC -O1   |       760 ms |     0% |
| RCC -O2   |       776 ms |     0% |
| TCC       |        97 ms |     2% |
| SLIMCC    |       648 ms |     1% |
| KEFIR     |     18309 ms |     0% |
| KEFIR -O1 |     33063 ms |     0% |
| ANTCC     |       387 ms |     0% |
| CCC       |     12625 ms |     0% |
| GCC -O0   |      4018 ms |     0% |
| GCC -O2   |     25530 ms |     0% |
| Clang -O0 |      1785 ms |     0% |
| Clang -O2 |     19849 ms |     0% |
