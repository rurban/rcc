# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           92 |          830 |        922 |    16% |
| RCC -O1   |           79 |          733 |        812 |    49% |
| RCC -O2   |           76 |          735 |        811 |    15% |
| TCC       |           39 |          633 |        672 |   125% |
| GCC -O0   |           70 |          505 |        575 |    41% |
| GCC -O2   |          113 |          330 |        443 |    22% |
| Clang -O0 |           91 |          535 |        626 |    30% |
| Clang -O2 |          122 |          304 |        426 |    14% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          187 |         6135 |       6322 |    25% |
| RCC -O1   |          210 |         4846 |       5056 |    20% |
| RCC -O2   |          130 |         4661 |       4791 |    25% |
| TCC       |          109 |         4346 |       4455 |    40% |
| GCC -O0   |          882 |         3890 |       4772 |    45% |
| GCC -O2   |         1207 |         2580 |       3787 |    15% |
| Clang -O0 |          551 |         3552 |       4103 |    28% |
| Clang -O2 |         1029 |         2154 |       3183 |    26% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    897 us
  parse       bench.c       :    390 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    204 us
  link        bench_rcc     :    156 us
  link        bench_rcc     :  81630 us

RCC -O1:
  preprocess  bench.c       :    728 us
  parse       bench.c       :    236 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     36 us
  codegen     bench.c       :    220 us
  link        bench_o1      :    214 us
  link        bench_o1      :  88062 us

RCC -O2:
  preprocess  bench.c       :    736 us
  parse       bench.c       :    204 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    157 us
  link        bench_o2      :    432 us
  link        bench_o2      :  76192 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 404693 us
  parse       sqlite3.c     : 102520 us
  typecheck   sqlite3.c     :  32525 us
  codegen     sqlite3.c     : 196544 us
  link        sqlite3.so    :  16760 us

RCC -O1:
  preprocess  sqlite3.c     : 295759 us
  parse       sqlite3.c     :  78813 us
  typecheck   sqlite3.c     :  13699 us
  opt         sqlite3.c     :  39201 us
  codegen     sqlite3.c     : 182545 us
  link        sqlite3.so    :  19073 us

RCC -O2:
  preprocess  sqlite3.c     : 372826 us
  parse       sqlite3.c     : 109783 us
  typecheck   sqlite3.c     :  16518 us
  opt         sqlite3.c     :  40971 us
  codegen     sqlite3.c     : 155072 us
  link        sqlite3.so    :  18953 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       529 ms |     1% |
| RCC -O1   |       473 ms |    23% |
| RCC -O2   |       524 ms |     9% |
| TCC       |       126 ms |     0% |
| GCC -O0   |      1515 ms |     2% |
| GCC -O2   |     11392 ms |     2% |
| Clang -O0 |      1221 ms |     7% |
| Clang -O2 |     11078 ms |    10% |
