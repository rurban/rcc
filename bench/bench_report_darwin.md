# Darwin RCC Benchmark Results

_Generated: October 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           38 |          719 |        757 |    26% |
| RCC -O1   |           32 |          776 |        808 |    11% |
| RCC -O2   |           35 |          747 |        782 |    20% |
| TCC       |           27 |          638 |        665 |    88% |
| GCC -O0   |           69 |          519 |        588 |    85% |
| GCC -O2   |          155 |          356 |        511 |   129% |
| Clang -O0 |           70 |          567 |        637 |    52% |
| Clang -O2 |          169 |          362 |        531 |   142% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          199 |         7179 |       7378 |    51% |
| RCC -O1   |          358 |         6413 |       6771 |    29% |
| RCC -O2   |          155 |         5948 |       6103 |   361% |
| TCC       |          121 |         6488 |       6609 |   140% |
| GCC -O0   |          728 |         4588 |       5316 |    45% |
| GCC -O2   |         1165 |         2060 |       3225 |    42% |
| Clang -O0 |          535 |         4180 |       4715 |    31% |
| Clang -O2 |         1416 |         2133 |       3549 |    32% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    878 us
  parse       bench.c       :    223 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    204 us
  link        bench_rcc     :  29704 us

RCC -O1:
  preprocess  bench.c       :    620 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     58 us
  codegen     bench.c       :    152 us
  link        bench_o1      :  26961 us

RCC -O2:
  preprocess  bench.c       :    642 us
  parse       bench.c       :    147 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     53 us
  codegen     bench.c       :    149 us
  link        bench_o2      :  26749 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 252728 us
  parse       sqlite3.c     :  68309 us
  typecheck   sqlite3.c     :  16140 us
  codegen     sqlite3.c     : 123490 us
  link        sqlite3.so    :  51094 us

RCC -O1:
  preprocess  sqlite3.c     : 351813 us
  parse       sqlite3.c     :  67981 us
  typecheck   sqlite3.c     :  16384 us
  opt         sqlite3.c     :  55389 us
  codegen     sqlite3.c     : 208683 us
  link        sqlite3.so    :  76223 us

RCC -O2:
  preprocess  sqlite3.c     : 376518 us
  parse       sqlite3.c     :  80926 us
  typecheck   sqlite3.c     :  13455 us
  opt         sqlite3.c     :  56792 us
  codegen     sqlite3.c     : 154224 us
  link        sqlite3.so    :  46855 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       640 ms |    63% |
| RCC -O1   |       583 ms |     6% |
| RCC -O2   |       603 ms |     0% |
| TCC       |       115 ms |    21% |
| GCC -O0   |      1238 ms |     4% |
| GCC -O2   |     12682 ms |     3% |
| Clang -O0 |      1175 ms |    12% |
| Clang -O2 |     14038 ms |     1% |
