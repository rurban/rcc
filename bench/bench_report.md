# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           14 |          700 |        714 |     7% |
| RCC -O1   |           14 |          701 |        715 |     7% |
| RCC -O2   |           14 |          730 |        744 |     7% |
| TCC       |            5 |          484 |        489 |     0% |
| SLIMCC    |           31 |          498 |        529 |     3% |
| XCC       |            8 |          357 |        365 |     0% |
| KEFIR     |          177 |          568 |        745 |     1% |
| KEFIR -O1 |          185 |          306 |        491 |     0% |
| SCC       |           32 |          535 |        567 |   484% |
| LACC      |           22 |          757 |        779 |    50% |
| ANTCC     |           23 |          413 |        436 |    13% |
| CAKE      |           88 |          477 |        565 |    45% |
| BCC       |           27 |          554 |        581 |   174% |
| CCC       |           32 |          536 |        568 |    16% |
| GCC -O0   |           53 |          478 |        531 |     3% |
| GCC -O2   |          145 |          175 |        320 |    39% |
| Clang -O0 |           77 |          463 |        540 |   823% |
| Clang -O2 |          126 |          176 |        302 |    85% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          274 |         5797 |       6071 |    16% |
| RCC -O1   |          276 |         6545 |       6821 |     0% |
| RCC -O2   |          276 |         6065 |       6341 |     1% |
| TCC       |           67 |         5038 |       5105 |    20% |
| SLIMCC    |          249 |         4893 |       5142 |     8% |
| KEFIR     |         4383 |         5278 |       9661 |     0% |
| KEFIR -O1 |         4835 |         3687 |       8522 |     0% |
| ANTCC     |          183 |         3712 |       3895 |     1% |
| CCC       |          860 |         4463 |       5323 |     1% |
| BCC       |         2970 |         4683 |       7653 |     2% |
| GCC -O0   |          830 |         4821 |       5651 |     1% |
| GCC -O2   |         1845 |         2518 |       4363 |     4% |
| Clang -O0 |          858 |         4565 |       5423 |     6% |
| Clang -O2 |         1575 |         2550 |       4125 |     3% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  10732 us
  parse       bench.c       :    642 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    316 us
  link        bench_rcc     :    912 us

RCC -O1:
  preprocess  bench.c       :  10373 us
  parse       bench.c       :    650 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     82 us
  codegen     bench.c       :    344 us
  link        bench_o1      :    867 us

RCC -O2:
  preprocess  bench.c       :  10282 us
  parse       bench.c       :    644 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    135 us
  codegen     bench.c       :    309 us
  link        bench_o2      :    859 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 252957 us
  parse       sqlite3.c     : 153650 us
  typecheck   sqlite3.c     :   8366 us
  codegen     sqlite3.c     : 205423 us
  link        sqlite3.so    :   8737 us

RCC -O1:
  preprocess  sqlite3.c     : 250638 us
  parse       sqlite3.c     : 150038 us
  typecheck   sqlite3.c     :   8427 us
  opt         sqlite3.c     :  64845 us
  codegen     sqlite3.c     : 202091 us
  link        sqlite3.so    :   9152 us

RCC -O2:
  preprocess  sqlite3.c     : 254637 us
  parse       sqlite3.c     : 149848 us
  typecheck   sqlite3.c     :   8192 us
  opt         sqlite3.c     :  75194 us
  codegen     sqlite3.c     : 204241 us
  link        sqlite3.so    :   8864 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       720 ms |     1% |
| RCC -O1   |       755 ms |     0% |
| RCC -O2   |       770 ms |     0% |
| TCC       |        97 ms |     8% |
| SLIMCC    |       647 ms |     1% |
| KEFIR     |     18341 ms |     0% |
| KEFIR -O1 |     33026 ms |     0% |
| ANTCC     |       387 ms |     0% |
| CCC       |     12680 ms |     0% |
| GCC -O0   |      4019 ms |     1% |
| GCC -O2   |     25543 ms |     0% |
| Clang -O0 |      1786 ms |     2% |
| Clang -O2 |     19864 ms |     0% |
