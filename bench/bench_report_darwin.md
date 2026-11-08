# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           71 |          852 |        923 |    16% |
| RCC -O1   |           96 |          931 |       1027 |    23% |
| RCC -O2   |           46 |          454 |        500 |   106% |
| RCC -O3   |           63 |          511 |        574 |    65% |
| TCC       |           50 |          756 |        806 |   162% |
| GCC -O0   |           98 |          642 |        740 |    76% |
| GCC -O2   |          185 |          355 |        540 |    17% |
| Clang -O0 |          100 |          513 |        613 |    34% |
| Clang -O2 |          149 |          441 |        590 |    36% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          336 |         5626 |       5962 |   146% |
| RCC -O1   |          169 |         5305 |       5474 |   162% |
| RCC -O2   |          215 |         2614 |       2829 |    15% |
| RCC -O3   |          215 |         2516 |       2731 |     2% |
| TCC       |          116 |         4945 |       5061 |    31% |
| GCC -O0   |          579 |         3751 |       4330 |    12% |
| GCC -O2   |         1016 |         2221 |       3237 |     6% |
| Clang -O0 |          558 |         3830 |       4388 |    11% |
| Clang -O2 |          968 |         2246 |       3214 |     3% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    821 us
  parse       bench.c       :    188 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    192 us
  link        bench_rcc     :  34137 us

RCC -O1:
  preprocess  bench.c       :    820 us
  parse       bench.c       :    184 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     58 us
  codegen     bench.c       :    187 us
  link        bench_o1      :  31010 us

RCC -O2:
  preprocess  bench.c       :    698 us
  parse       bench.c       :    182 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     55 us
  codegen     bench.c       :   2064 us
  link        bench_o2      :  29986 us

RCC -O3:
  preprocess  bench.c       :    570 us
  parse       bench.c       :    155 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     56 us
  codegen     bench.c       :   1682 us
  link        bench_o3      :  26596 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 296186 us
  parse       sqlite3.c     : 140394 us
  typecheck   sqlite3.c     :  25265 us
  codegen     sqlite3.c     : 148766 us
  link        sqlite3.so    :  53869 us

RCC -O1:
  preprocess  sqlite3.c     : 276116 us
  parse       sqlite3.c     :  58735 us
  typecheck   sqlite3.c     :  12431 us
  opt         sqlite3.c     :  49076 us
  codegen     sqlite3.c     : 135121 us
  link        sqlite3.so    :  38873 us

RCC -O2:
  preprocess  sqlite3.c     : 210177 us
  parse       sqlite3.c     :  54860 us
  typecheck   sqlite3.c     :  12987 us
  opt         sqlite3.c     :  58933 us
  codegen     sqlite3.c     : 1156103 us
  link        sqlite3.so    :  43490 us

RCC -O3:
  preprocess  sqlite3.c     : 251961 us
  parse       sqlite3.c     :  63083 us
  typecheck   sqlite3.c     :  12839 us
  opt         sqlite3.c     :  52212 us
  codegen     sqlite3.c     : 1589635 us
  link        sqlite3.so    :  66635 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       610 ms |    96% |
| RCC -O1   |       523 ms |     3% |
| RCC -O2   |      1517 ms |     4% |
| RCC -O3   |      1400 ms |     4% |
| TCC       |        94 ms |    19% |
| GCC -O0   |      1123 ms |    16% |
| GCC -O2   |     11028 ms |     2% |
| Clang -O0 |      1116 ms |     4% |
| Clang -O2 |     10954 ms |     0% |
