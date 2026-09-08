# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           24 |          715 |        739 |     5% |
| RCC -O1   |           25 |          741 |        766 |    24% |
| RCC -O2   |           26 |          711 |        737 |     9% |
| TCC       |            5 |          497 |        502 |    20% |
| SLIMCC    |           33 |          520 |        553 |     3% |
| XCC       |            9 |          361 |        370 |    22% |
| KEFIR     |          182 |          588 |        770 |     8% |
| KEFIR -O1 |          196 |          311 |        507 |     9% |
| SCC       |           32 |          554 |        586 |    19% |
| LACC      |           25 |          780 |        805 |     4% |
| ANTCC     |           25 |          428 |        453 |     6% |
| CAKE      |           89 |          486 |        575 |     8% |
| CCC       |           34 |          553 |        587 |    21% |
| GCC -O0   |           54 |          486 |        540 |     5% |
| GCC -O2   |          148 |          177 |        325 |     2% |
| Clang -O0 |           77 |          476 |        553 |     4% |
| Clang -O2 |          134 |          180 |        314 |    11% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          387 |         6140 |       6527 |     8% |
| RCC -O1   |          394 |         6983 |       7377 |    13% |
| RCC -O2   |          400 |         6411 |       6811 |     4% |
| TCC       |           66 |         5297 |       5363 |     6% |
| SLIMCC    |          259 |         5139 |       5398 |     2% |
| KEFIR     |         4516 |         5465 |       9981 |     1% |
| KEFIR -O1 |         5003 |         3833 |       8836 |     5% |
| ANTCC     |          191 |         3853 |       4044 |     6% |
| CCC       |          876 |         4742 |       5618 |     5% |
| GCC -O0   |          886 |         5257 |       6143 |     2% |
| GCC -O2   |         1985 |         2685 |       4670 |     7% |
| Clang -O0 |          889 |         4830 |       5719 |    10% |
| Clang -O2 |         1746 |         3519 |       5265 |    23% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   9756 us
  parse       bench.c       :    639 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    297 us
  link        bench_rcc     :  10873 us

RCC -O1:
  preprocess  bench.c       :   9658 us
  parse       bench.c       :    725 us
  typecheck   bench.c       :     23 us
  opt         bench.c       :     62 us
  codegen     bench.c       :    332 us
  link        bench_o1      :  11732 us

RCC -O2:
  preprocess  bench.c       :   9910 us
  parse       bench.c       :    653 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     52 us
  codegen     bench.c       :    357 us
  link        bench_o2      :  11981 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 277898 us
  parse       sqlite3.c     : 152438 us
  typecheck   sqlite3.c     :   8650 us
  codegen     sqlite3.c     : 205280 us
  link        sqlite3.so    :   9623 us

RCC -O1:
  preprocess  sqlite3.c     : 253440 us
  parse       sqlite3.c     : 155261 us
  typecheck   sqlite3.c     :   8653 us
  opt         sqlite3.c     :  35293 us
  codegen     sqlite3.c     : 202559 us
  link        sqlite3.so    :  11052 us

RCC -O2:
  preprocess  sqlite3.c     : 262238 us
  parse       sqlite3.c     : 163122 us
  typecheck   sqlite3.c     :   8828 us
  opt         sqlite3.c     :  47462 us
  codegen     sqlite3.c     : 210236 us
  link        sqlite3.so    :  10342 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       889 ms |     0% |
| RCC -O1   |       873 ms |     0% |
| RCC -O2   |       876 ms |     3% |
| TCC       |       120 ms |     2% |
| SLIMCC    |       801 ms |    13% |
| KEFIR     |     19221 ms |    19% |
| KEFIR -O1 |     34448 ms |     2% |
| ANTCC     |       400 ms |     7% |
| CCC       |     13086 ms |     1% |
| GCC -O0   |      4162 ms |     1% |
| GCC -O2   |     26555 ms |     0% |
| Clang -O0 |      1840 ms |     1% |
| Clang -O2 |     20599 ms |     1% |
