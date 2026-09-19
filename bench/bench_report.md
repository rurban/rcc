# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           14 |          701 |        715 |     7% |
| RCC -O1   |           14 |          700 |        714 |     4% |
| RCC -O2   |           14 |          732 |        746 |    50% |
| TCC       |            5 |          485 |        490 |     0% |
| SLIMCC    |           31 |          499 |        530 |     3% |
| XCC       |            8 |          357 |        365 |    12% |
| KEFIR     |          175 |          569 |        744 |     0% |
| KEFIR -O1 |          185 |          306 |        491 |     0% |
| SCC       |           32 |          610 |        642 |     0% |
| LACC      |           21 |          755 |        776 |     4% |
| ANTCC     |           23 |          412 |        435 |     4% |
| CAKE      |           89 |          477 |        566 |     0% |
| BCC       |           27 |          558 |        585 |     0% |
| CCC       |           33 |          536 |        569 |    16% |
| GCC -O0   |           53 |          477 |        530 |     1% |
| GCC -O2   |          143 |          174 |        317 |     2% |
| Clang -O0 |           76 |          461 |        537 |     1% |
| Clang -O2 |          125 |          176 |        301 |     0% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          274 |         5822 |       6096 |     1% |
| RCC -O1   |          275 |         6542 |       6817 |     1% |
| RCC -O2   |          273 |         6060 |       6333 |     2% |
| TCC       |           65 |         5040 |       5105 |     1% |
| SLIMCC    |          249 |         4889 |       5138 |     0% |
| KEFIR     |         4363 |         5276 |       9639 |     1% |
| KEFIR -O1 |         4813 |         3669 |       8482 |     0% |
| ANTCC     |          185 |         3710 |       3895 |     1% |
| CCC       |          854 |         4466 |       5320 |     1% |
| BCC       |         2962 |         4637 |       7599 |     2% |
| GCC -O0   |          832 |         4800 |       5632 |     1% |
| GCC -O2   |         1844 |         2515 |       4359 |     1% |
| Clang -O0 |          870 |         4570 |       5440 |     0% |
| Clang -O2 |         1583 |         2543 |       4126 |     0% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  10307 us
  parse       bench.c       :    588 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    363 us
  link        bench_rcc     :    878 us

RCC -O1:
  preprocess  bench.c       :  10276 us
  parse       bench.c       :    623 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    113 us
  codegen     bench.c       :    318 us
  link        bench_o1      :    866 us

RCC -O2:
  preprocess  bench.c       :  10377 us
  parse       bench.c       :    576 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     80 us
  codegen     bench.c       :    371 us
  link        bench_o2      :    863 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 251069 us
  parse       sqlite3.c     : 153117 us
  typecheck   sqlite3.c     :   8699 us
  codegen     sqlite3.c     : 210311 us
  link        sqlite3.so    :   8983 us

RCC -O1:
  preprocess  sqlite3.c     : 252041 us
  parse       sqlite3.c     : 150291 us
  typecheck   sqlite3.c     :   8220 us
  opt         sqlite3.c     :  64621 us
  codegen     sqlite3.c     : 210973 us
  link        sqlite3.so    :   9176 us

RCC -O2:
  preprocess  sqlite3.c     : 249526 us
  parse       sqlite3.c     : 150258 us
  typecheck   sqlite3.c     :   8323 us
  opt         sqlite3.c     :  74279 us
  codegen     sqlite3.c     : 208620 us
  link        sqlite3.so    :   9107 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       728 ms |     0% |
| RCC -O1   |       762 ms |     0% |
| RCC -O2   |       775 ms |     0% |
| TCC       |        97 ms |     1% |
| SLIMCC    |       648 ms |     1% |
| KEFIR     |     18309 ms |     0% |
| KEFIR -O1 |     33042 ms |     0% |
| ANTCC     |       389 ms |     1% |
| CCC       |     12620 ms |     0% |
| GCC -O0   |      4028 ms |     0% |
| GCC -O2   |     25559 ms |     0% |
| Clang -O0 |      1778 ms |     0% |
| Clang -O2 |     19829 ms |     0% |
