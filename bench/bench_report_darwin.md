# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           56 |          654 |        710 |     9% |
| RCC -O1   |           58 |          659 |        717 |    87% |
| RCC -O2   |           66 |          660 |        726 |    16% |
| TCC       |           32 |          590 |        622 |   140% |
| GCC -O0   |           68 |          489 |        557 |    27% |
| GCC -O2   |          108 |          308 |        416 |    20% |
| Clang -O0 |           61 |          508 |        569 |    21% |
| Clang -O2 |           93 |          292 |        385 |    75% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          157 |         5229 |       5386 |    12% |
| RCC -O1   |          134 |         5169 |       5303 |    48% |
| RCC -O2   |          146 |         4914 |       5060 |    66% |
| TCC       |          129 |         4938 |       5067 |    62% |
| GCC -O0   |          520 |         3962 |       4482 |    20% |
| GCC -O2   |          884 |         2359 |       3243 |    10% |
| Clang -O0 |          505 |         3685 |       4190 |     5% |
| Clang -O2 |          879 |         2147 |       3026 |    28% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    689 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    119 us
  link        bench_rcc     :    118 us
  link        bench_rcc     :  60057 us

RCC -O1:
  preprocess  bench.c       :    804 us
  parse       bench.c       :    176 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    154 us
  link        bench_o1      :    105 us
  link        bench_o1      :  71138 us

RCC -O2:
  preprocess  bench.c       :    643 us
  parse       bench.c       :    164 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    141 us
  link        bench_o2      :     98 us
  link        bench_o2      :  57922 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 238313 us
  parse       sqlite3.c     :  57128 us
  typecheck   sqlite3.c     :  11593 us
  codegen     sqlite3.c     : 106246 us
  link        sqlite3.so    :  14901 us

RCC -O1:
  preprocess  sqlite3.c     : 206187 us
  parse       sqlite3.c     :  50463 us
  typecheck   sqlite3.c     :  10970 us
  opt         sqlite3.c     :  24205 us
  codegen     sqlite3.c     :  98626 us
  link        sqlite3.so    :  14777 us

RCC -O2:
  preprocess  sqlite3.c     : 195874 us
  parse       sqlite3.c     :  51825 us
  typecheck   sqlite3.c     :  11102 us
  opt         sqlite3.c     :  27527 us
  codegen     sqlite3.c     :  99569 us
  link        sqlite3.so    :  15383 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       500 ms |    62% |
| RCC -O1   |       441 ms |    22% |
| RCC -O2   |       447 ms |     0% |
| TCC       |        93 ms |    22% |
| GCC -O0   |      1012 ms |     1% |
| GCC -O2   |      9797 ms |    13% |
| Clang -O0 |      1126 ms |    10% |
| Clang -O2 |      9881 ms |     3% |
