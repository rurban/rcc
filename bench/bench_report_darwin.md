# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           41 |          838 |        879 |    19% |
| RCC -O1   |           48 |          824 |        872 |    40% |
| RCC -O2   |           36 |          428 |        464 |    50% |
| RCC -O3   |           31 |          439 |        470 |    35% |
| TCC       |           39 |          682 |        721 |   161% |
| GCC -O0   |           77 |          505 |        582 |    77% |
| GCC -O2   |          138 |          332 |        470 |    23% |
| Clang -O0 |           84 |          572 |        656 |    27% |
| Clang -O2 |          123 |          300 |        423 |    54% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          141 |         6246 |       6387 |    75% |
| RCC -O1   |          196 |         5557 |       5753 |    47% |
| RCC -O2   |          192 |         2603 |       2795 |    73% |
| RCC -O3   |          212 |         2166 |       2378 |    79% |
| TCC       |          105 |         5341 |       5446 |    45% |
| GCC -O0   |          591 |         3805 |       4396 |    27% |
| GCC -O2   |         1048 |         2374 |       3422 |    10% |
| Clang -O0 |          581 |         3848 |       4429 |    18% |
| Clang -O2 |         1015 |         2039 |       3054 |     5% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    954 us
  parse       bench.c       :    132 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    166 us
  link        bench_rcc     :  25987 us

RCC -O1:
  preprocess  bench.c       :    616 us
  parse       bench.c       :    170 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     54 us
  codegen     bench.c       :    149 us
  link        bench_o1      :  27659 us

RCC -O2:
  preprocess  bench.c       :    579 us
  parse       bench.c       :    134 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     53 us
  codegen     bench.c       :   1648 us
  link        bench_o2      :  27482 us

RCC -O3:
  preprocess  bench.c       :    804 us
  parse       bench.c       :    183 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     57 us
  codegen     bench.c       :   1683 us
  link        bench_o3      :  33129 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 253604 us
  parse       sqlite3.c     : 193187 us
  typecheck   sqlite3.c     :  25919 us
  codegen     sqlite3.c     : 172214 us
  link        sqlite3.so    :  40157 us

RCC -O1:
  preprocess  sqlite3.c     : 259824 us
  parse       sqlite3.c     :  57400 us
  typecheck   sqlite3.c     :  16292 us
  opt         sqlite3.c     :  48826 us
  codegen     sqlite3.c     : 143630 us
  link        sqlite3.so    :  46153 us

RCC -O2:
  preprocess  sqlite3.c     : 246624 us
  parse       sqlite3.c     :  57561 us
  typecheck   sqlite3.c     :  12730 us
  opt         sqlite3.c     :  55260 us
  codegen     sqlite3.c     : 1286390 us
  link        sqlite3.so    :  68835 us

RCC -O3:
  preprocess  sqlite3.c     : 338016 us
  parse       sqlite3.c     :  90468 us
  typecheck   sqlite3.c     :  19646 us
  opt         sqlite3.c     :  86905 us
  codegen     sqlite3.c     : 1375115 us
  link        sqlite3.so    :  48583 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       539 ms |    19% |
| RCC -O1   |       543 ms |     2% |
| RCC -O2   |      1595 ms |     5% |
| RCC -O3   |      1621 ms |     4% |
| TCC       |       117 ms |     6% |
| GCC -O0   |      1370 ms |    15% |
| GCC -O2   |     12953 ms |    23% |
| Clang -O0 |      1023 ms |     9% |
| Clang -O2 |     10581 ms |     3% |
