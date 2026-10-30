# Darwin RCC Benchmark Results

_Generated: October 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           44 |          717 |        761 |    25% |
| RCC -O1   |           29 |          712 |        741 |     7% |
| RCC -O2   |           37 |          832 |        869 |    21% |
| TCC       |           35 |          704 |        739 |   151% |
| GCC -O0   |           87 |          682 |        769 |    93% |
| GCC -O2   |          171 |          401 |        572 |    42% |
| Clang -O0 |          100 |          685 |        785 |    34% |
| Clang -O2 |          191 |          420 |        611 |    75% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          288 |         6483 |       6771 |    38% |
| RCC -O1   |          199 |         6476 |       6675 |   146% |
| RCC -O2   |          251 |         5103 |       5354 |    54% |
| TCC       |          139 |         5603 |       5742 |    80% |
| GCC -O0   |          712 |         4140 |       4852 |    12% |
| GCC -O2   |         1141 |         2534 |       3675 |    34% |
| Clang -O0 |          799 |         3872 |       4671 |    37% |
| Clang -O2 |          811 |         2001 |       2812 |    29% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2235 us
  parse       bench.c       :    302 us
  typecheck   bench.c       :      8 us
  codegen     bench.c       :    300 us
  link        bench_rcc     : 103580 us

RCC -O1:
  preprocess  bench.c       :    727 us
  parse       bench.c       :    214 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     58 us
  codegen     bench.c       :    225 us
  link        bench_o1      :  31431 us

RCC -O2:
  preprocess  bench.c       :    637 us
  parse       bench.c       :    134 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     47 us
  codegen     bench.c       :    131 us
  link        bench_o2      :  46628 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 410077 us
  parse       sqlite3.c     : 247020 us
  typecheck   sqlite3.c     :  24439 us
  codegen     sqlite3.c     : 216118 us
  link        sqlite3.so    :  55619 us

RCC -O1:
  preprocess  sqlite3.c     : 328155 us
  parse       sqlite3.c     :  93746 us
  typecheck   sqlite3.c     :  32472 us
  opt         sqlite3.c     : 117378 us
  codegen     sqlite3.c     : 158055 us
  link        sqlite3.so    :  36015 us

RCC -O2:
  preprocess  sqlite3.c     : 293396 us
  parse       sqlite3.c     :  79262 us
  typecheck   sqlite3.c     :  20264 us
  opt         sqlite3.c     :  63815 us
  codegen     sqlite3.c     : 214739 us
  link        sqlite3.so    :  59729 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       504 ms |    67% |
| RCC -O1   |       471 ms |     4% |
| RCC -O2   |       463 ms |     7% |
| TCC       |       140 ms |    21% |
| GCC -O0   |      1036 ms |     4% |
| GCC -O2   |     12236 ms |    37% |
| Clang -O0 |      1970 ms |    20% |
| Clang -O2 |     10900 ms |    18% |
