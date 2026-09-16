# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           54 |          697 |        751 |    18% |
| RCC -O1   |           57 |          664 |        721 |     5% |
| RCC -O2   |           61 |          664 |        725 |     5% |
| TCC       |           26 |          561 |        587 |    73% |
| GCC -O0   |           66 |          434 |        500 |     5% |
| GCC -O2   |           87 |          262 |        349 |     8% |
| Clang -O0 |           53 |          435 |        488 |     0% |
| Clang -O2 |           82 |          265 |        347 |     8% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          117 |         4389 |       4506 |    13% |
| RCC -O1   |          117 |         4469 |       4586 |    25% |
| RCC -O2   |          117 |         3913 |       4030 |     4% |
| TCC       |           98 |         3733 |       3831 |    11% |
| GCC -O0   |          457 |         3079 |       3536 |     9% |
| GCC -O2   |          787 |         1751 |       2538 |     6% |
| Clang -O0 |          454 |         3079 |       3533 |     7% |
| Clang -O2 |          799 |         1740 |       2539 |     8% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    692 us
  parse       bench.c       :    163 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    128 us
  link        bench_rcc     :    138 us
  link        bench_rcc     :  55124 us

RCC -O1:
  preprocess  bench.c       :    568 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    132 us
  link        bench_o1      :    172 us
  link        bench_o1      :  49610 us

RCC -O2:
  preprocess  bench.c       :    565 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    141 us
  link        bench_o2      :    144 us
  link        bench_o2      :  47038 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 209568 us
  parse       sqlite3.c     :  48877 us
  typecheck   sqlite3.c     :  11991 us
  codegen     sqlite3.c     : 112123 us
  link        sqlite3.so    :  15310 us

RCC -O1:
  preprocess  sqlite3.c     : 197180 us
  parse       sqlite3.c     :  54415 us
  typecheck   sqlite3.c     :  12523 us
  opt         sqlite3.c     :  23815 us
  codegen     sqlite3.c     : 114730 us
  link        sqlite3.so    :  15358 us

RCC -O2:
  preprocess  sqlite3.c     : 209523 us
  parse       sqlite3.c     :  58541 us
  typecheck   sqlite3.c     :  12246 us
  opt         sqlite3.c     :  27296 us
  codegen     sqlite3.c     : 112898 us
  link        sqlite3.so    :  15771 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       429 ms |     6% |
| RCC -O1   |       393 ms |     1% |
| RCC -O2   |       404 ms |     0% |
| TCC       |        85 ms |     7% |
| GCC -O0   |       916 ms |     1% |
| GCC -O2   |      8835 ms |     0% |
| Clang -O0 |       904 ms |     0% |
| Clang -O2 |      8687 ms |     2% |
