# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           31 |          706 |        737 |    35% |
| RCC -O1   |           37 |          787 |        824 |    21% |
| RCC -O2   |           42 |          762 |        804 |    23% |
| RCC -O3   |           47 |          832 |        879 |   308% |
| TCC       |           32 |          760 |        792 |   284% |
| GCC -O0   |           96 |          548 |        644 |    89% |
| GCC -O2   |          145 |          324 |        469 |    15% |
| Clang -O0 |           72 |          545 |        617 |    58% |
| Clang -O2 |          113 |          315 |        428 |    49% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          256 |         6829 |       7085 |   124% |
| RCC -O1   |          161 |         6633 |       6794 |    43% |
| RCC -O2   |          200 |         4893 |       5093 |    98% |
| RCC -O3   |          219 |         4489 |       4708 |   163% |
| TCC       |          157 |         4772 |       4929 |    54% |
| GCC -O0   |          574 |         4373 |       4947 |    50% |
| GCC -O2   |         1095 |         2816 |       3911 |    22% |
| Clang -O0 |          630 |         3894 |       4524 |    55% |
| Clang -O2 |         1444 |         2502 |       3946 |    22% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    786 us
  parse       bench.c       :    200 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    210 us
  link        bench_rcc     :  28697 us

RCC -O1:
  preprocess  bench.c       :   1328 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     53 us
  codegen     bench.c       :    142 us
  link        bench_o1      :  32789 us

RCC -O2:
  preprocess  bench.c       :    571 us
  parse       bench.c       :    127 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     50 us
  codegen     bench.c       :    155 us
  link        bench_o2      :  26180 us

RCC -O3:
  preprocess  bench.c       :    747 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :    110 us
  codegen     bench.c       :    185 us
  link        bench_o3      :  29852 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 259136 us
  parse       sqlite3.c     : 171666 us
  typecheck   sqlite3.c     :  19542 us
  codegen     sqlite3.c     : 130116 us
  link        sqlite3.so    :  53664 us

RCC -O1:
  preprocess  sqlite3.c     : 310270 us
  parse       sqlite3.c     :  90342 us
  typecheck   sqlite3.c     :  18094 us
  opt         sqlite3.c     :  72230 us
  codegen     sqlite3.c     : 146846 us
  link        sqlite3.so    :  52221 us

RCC -O2:
  preprocess  sqlite3.c     : 354439 us
  parse       sqlite3.c     :  72804 us
  typecheck   sqlite3.c     :  12550 us
  opt         sqlite3.c     :  61370 us
  codegen     sqlite3.c     : 167196 us
  link        sqlite3.so    :  40363 us

RCC -O3:
  preprocess  sqlite3.c     : 323623 us
  parse       sqlite3.c     :  69474 us
  typecheck   sqlite3.c     :  19063 us
  opt         sqlite3.c     :  76248 us
  codegen     sqlite3.c     : 197123 us
  link        sqlite3.so    :  41144 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       581 ms |    25% |
| RCC -O1   |       471 ms |    12% |
| RCC -O2   |       577 ms |     0% |
| RCC -O3   |       537 ms |     5% |
| TCC       |       115 ms |   102% |
| GCC -O0   |      1201 ms |     7% |
| GCC -O2   |     10555 ms |    14% |
| Clang -O0 |      1172 ms |     9% |
| Clang -O2 |     12611 ms |    19% |
