# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           34 |          649 |        683 |     2% |
| RCC -O1   |           31 |          671 |        702 |     4% |
| RCC -O2   |           32 |          369 |        401 |     9% |
| RCC -O3   |           34 |          376 |        410 |    88% |
| TCC       |           31 |          595 |        626 |    54% |
| GCC -O0   |           77 |          469 |        546 |    46% |
| GCC -O2   |           90 |          264 |        354 |    27% |
| Clang -O0 |           53 |          433 |        486 |     9% |
| Clang -O2 |           83 |          260 |        343 |     7% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          119 |         4149 |       4268 |    12% |
| RCC -O1   |          132 |         4101 |       4233 |    18% |
| RCC -O2   |          150 |         1929 |       2079 |     9% |
| RCC -O3   |          177 |         2281 |       2458 |     6% |
| TCC       |          109 |         3900 |       4009 |     7% |
| GCC -O0   |          549 |         2825 |       3374 |    15% |
| GCC -O2   |          708 |         1810 |       2518 |    26% |
| Clang -O0 |          488 |         3221 |       3709 |    11% |
| Clang -O2 |          711 |         1567 |       2278 |    10% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1219 us
  parse       bench.c       :    307 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    187 us
  link        bench_rcc     :  26147 us

RCC -O1:
  preprocess  bench.c       :    669 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     53 us
  codegen     bench.c       :    166 us
  link        bench_o1      :  27862 us

RCC -O2:
  preprocess  bench.c       :    782 us
  parse       bench.c       :    183 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     57 us
  codegen     bench.c       :   1659 us
  link        bench_o2      :  25834 us

RCC -O3:
  preprocess  bench.c       :    573 us
  parse       bench.c       :    169 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     57 us
  codegen     bench.c       :   1793 us
  link        bench_o3      :  27072 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 200941 us
  parse       sqlite3.c     :  57294 us
  typecheck   sqlite3.c     :  14431 us
  codegen     sqlite3.c     : 132554 us
  link        sqlite3.so    :  32716 us

RCC -O1:
  preprocess  sqlite3.c     : 190969 us
  parse       sqlite3.c     :  53358 us
  typecheck   sqlite3.c     :  14673 us
  opt         sqlite3.c     :  52613 us
  codegen     sqlite3.c     : 131797 us
  link        sqlite3.so    :  31312 us

RCC -O2:
  preprocess  sqlite3.c     : 201295 us
  parse       sqlite3.c     :  51809 us
  typecheck   sqlite3.c     :  14880 us
  opt         sqlite3.c     :  59195 us
  codegen     sqlite3.c     : 934374 us
  link        sqlite3.so    :  30393 us

RCC -O3:
  preprocess  sqlite3.c     : 217090 us
  parse       sqlite3.c     :  54549 us
  typecheck   sqlite3.c     :  15527 us
  opt         sqlite3.c     :  63553 us
  codegen     sqlite3.c     : 1012742 us
  link        sqlite3.so    :  31156 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       394 ms |    21% |
| RCC -O1   |       396 ms |     0% |
| RCC -O2   |      1056 ms |     1% |
| RCC -O3   |      1054 ms |     0% |
| TCC       |        75 ms |     1% |
| GCC -O0   |       866 ms |     0% |
| GCC -O2   |      7693 ms |     0% |
| Clang -O0 |       805 ms |     0% |
| Clang -O2 |      7959 ms |     3% |
