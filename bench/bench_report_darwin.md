# Darwin RCC Benchmark Results

_Generated: October 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           42 |          767 |        809 |    14% |
| RCC -O1   |           44 |          763 |        807 |    20% |
| RCC -O2   |           38 |          698 |        736 |    18% |
| TCC       |           28 |          568 |        596 |   117% |
| GCC -O0   |           80 |          456 |        536 |    66% |
| GCC -O2   |          107 |          292 |        399 |    13% |
| Clang -O0 |           56 |          460 |        516 |    25% |
| Clang -O2 |           90 |          302 |        392 |     4% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          162 |         6229 |       6391 |    30% |
| RCC -O1   |          188 |         5166 |       5354 |    61% |
| RCC -O2   |          140 |         4584 |       4724 |    27% |
| TCC       |          123 |         4588 |       4711 |    32% |
| GCC -O0   |          556 |         3582 |       4138 |    31% |
| GCC -O2   |         1258 |         2974 |       4232 |    14% |
| Clang -O0 |          654 |         3658 |       4312 |    19% |
| Clang -O2 |          859 |         2127 |       2986 |    15% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    831 us
  parse       bench.c       :    195 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    196 us
  link        bench_rcc     :  30988 us

RCC -O1:
  preprocess  bench.c       :    846 us
  parse       bench.c       :    207 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     53 us
  codegen     bench.c       :    174 us
  link        bench_o1      :  34331 us

RCC -O2:
  preprocess  bench.c       :    882 us
  parse       bench.c       :    240 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     59 us
  codegen     bench.c       :    206 us
  link        bench_o2      :  44900 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 314732 us
  parse       sqlite3.c     :  85965 us
  typecheck   sqlite3.c     :  16392 us
  codegen     sqlite3.c     : 163106 us
  link        sqlite3.so    :  58169 us

RCC -O1:
  preprocess  sqlite3.c     : 308652 us
  parse       sqlite3.c     :  79324 us
  typecheck   sqlite3.c     :  19350 us
  opt         sqlite3.c     :  50080 us
  codegen     sqlite3.c     : 171290 us
  link        sqlite3.so    :  57851 us

RCC -O2:
  preprocess  sqlite3.c     : 328266 us
  parse       sqlite3.c     :  66262 us
  typecheck   sqlite3.c     :  25640 us
  opt         sqlite3.c     :  83421 us
  codegen     sqlite3.c     : 135033 us
  link        sqlite3.so    :  52906 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       636 ms |    19% |
| RCC -O1   |       443 ms |    33% |
| RCC -O2   |       463 ms |    32% |
| TCC       |       102 ms |     9% |
| GCC -O0   |       988 ms |     1% |
| GCC -O2   |     10421 ms |     7% |
| Clang -O0 |      1400 ms |    12% |
| Clang -O2 |     13363 ms |    12% |
