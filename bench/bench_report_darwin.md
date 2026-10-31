# Darwin RCC Benchmark Results

_Generated: October 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           27 |          635 |        662 |     5% |
| RCC -O1   |           32 |          642 |        674 |     9% |
| RCC -O2   |           30 |          637 |        667 |    17% |
| TCC       |           32 |          637 |        669 |   196% |
| GCC -O0   |           71 |          486 |        557 |    59% |
| GCC -O2   |          135 |          294 |        429 |    14% |
| Clang -O0 |           77 |          483 |        560 |     7% |
| Clang -O2 |           98 |          275 |        373 |    20% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          145 |         5007 |       5152 |    30% |
| RCC -O1   |          195 |         4888 |       5083 |    33% |
| RCC -O2   |          160 |         4271 |       4431 |    43% |
| TCC       |           99 |         3790 |       3889 |    17% |
| GCC -O0   |          456 |         3081 |       3537 |     9% |
| GCC -O2   |          785 |         1741 |       2526 |    18% |
| Clang -O0 |          562 |         3925 |       4487 |    10% |
| Clang -O2 |          886 |         2043 |       2929 |    16% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    854 us
  parse       bench.c       :    168 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    203 us
  link        bench_rcc     :  29117 us

RCC -O1:
  preprocess  bench.c       :    606 us
  parse       bench.c       :    180 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     50 us
  codegen     bench.c       :    163 us
  link        bench_o1      :  27024 us

RCC -O2:
  preprocess  bench.c       :    611 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     50 us
  codegen     bench.c       :    140 us
  link        bench_o2      :  26730 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 239037 us
  parse       sqlite3.c     :  64559 us
  typecheck   sqlite3.c     :  20659 us
  codegen     sqlite3.c     : 114640 us
  link        sqlite3.so    :  47276 us

RCC -O1:
  preprocess  sqlite3.c     : 239865 us
  parse       sqlite3.c     :  51161 us
  typecheck   sqlite3.c     :  10235 us
  opt         sqlite3.c     :  38808 us
  codegen     sqlite3.c     :  99124 us
  link        sqlite3.so    :  28393 us

RCC -O2:
  preprocess  sqlite3.c     : 191466 us
  parse       sqlite3.c     :  50361 us
  typecheck   sqlite3.c     :  10184 us
  opt         sqlite3.c     :  44049 us
  codegen     sqlite3.c     :  99680 us
  link        sqlite3.so    :  29049 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       546 ms |    38% |
| RCC -O1   |       529 ms |    18% |
| RCC -O2   |       556 ms |     5% |
| TCC       |        91 ms |    41% |
| GCC -O0   |      1114 ms |    13% |
| GCC -O2   |     13309 ms |     0% |
| Clang -O0 |      1339 ms |     8% |
| Clang -O2 |      9874 ms |    17% |
