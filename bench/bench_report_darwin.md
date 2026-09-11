# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           54 |          601 |        655 |    12% |
| RCC -O1   |           53 |          600 |        653 |     7% |
| RCC -O2   |           46 |          621 |        667 |    17% |
| TCC       |           25 |          523 |        548 |    48% |
| GCC -O0   |           69 |          481 |        550 |     2% |
| GCC -O2   |           90 |          268 |        358 |    16% |
| Clang -O0 |           65 |          432 |        497 |     5% |
| Clang -O2 |           76 |          256 |        332 |     3% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          115 |         3922 |       4037 |     4% |
| RCC -O1   |          106 |         3916 |       4022 |     7% |
| RCC -O2   |          104 |         3434 |       3538 |     3% |
| TCC       |           88 |         3354 |       3442 |     9% |
| GCC -O0   |          383 |         2754 |       3137 |     1% |
| GCC -O2   |          674 |         1580 |       2254 |    17% |
| Clang -O0 |          373 |         2806 |       3179 |     9% |
| Clang -O2 |          663 |         1560 |       2223 |     2% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    691 us
  parse       bench.c       :    142 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    132 us
  link        bench_rcc     :    104 us
  link        bench_rcc     :  48140 us

RCC -O1:
  preprocess  bench.c       :    489 us
  parse       bench.c       :    117 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    114 us
  link        bench_o1      :     65 us
  link        bench_o1      :  40056 us

RCC -O2:
  preprocess  bench.c       :    523 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    109 us
  link        bench_o2      :    101 us
  link        bench_o2      :  40702 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 172762 us
  parse       sqlite3.c     :  48722 us
  typecheck   sqlite3.c     :  12625 us
  codegen     sqlite3.c     : 114640 us
  link        sqlite3.so    :  14669 us

RCC -O1:
  preprocess  sqlite3.c     : 197669 us
  parse       sqlite3.c     :  50947 us
  typecheck   sqlite3.c     :  14476 us
  opt         sqlite3.c     :  26191 us
  codegen     sqlite3.c     :  97781 us
  link        sqlite3.so    :  11932 us

RCC -O2:
  preprocess  sqlite3.c     : 182471 us
  parse       sqlite3.c     :  53641 us
  typecheck   sqlite3.c     :  16802 us
  opt         sqlite3.c     :  31256 us
  codegen     sqlite3.c     : 120086 us
  link        sqlite3.so    :  14986 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       352 ms |     2% |
| RCC -O1   |       374 ms |     6% |
| RCC -O2   |       352 ms |     1% |
| TCC       |        75 ms |     2% |
| GCC -O0   |       824 ms |     3% |
| GCC -O2   |      7673 ms |     0% |
| Clang -O0 |       826 ms |     6% |
| Clang -O2 |      7713 ms |     5% |
