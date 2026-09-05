# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           57 |          634 |        691 |
| RCC -O1   |           59 |          601 |        660 |
| RCC -O2   |           51 |          685 |        736 |
| TCC       |           84 |          568 |        652 |
| GCC -O0   |          107 |          518 |        625 |
| GCC -O2   |          109 |          269 |        378 |
| Clang -O0 |           55 |          458 |        513 |
| Clang -O2 |          104 |          294 |        398 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          179 |         6737 |       6916 |
| RCC -O1   |          225 |         5332 |       5557 |
| RCC -O2   |          181 |         4665 |       4846 |
| TCC       |          168 |         4596 |       4764 |
| GCC -O0   |          608 |         3422 |       4030 |
| GCC -O2   |          863 |         1981 |       2844 |
| Clang -O0 |          571 |         3272 |       3843 |
| Clang -O2 |          902 |         1751 |       2653 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    821 us
  parse       bench.c       :    212 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    187 us
  link        bench_rcc     :    154 us
  link        bench_rcc     :  55937 us

RCC -O1:
  preprocess  bench.c       :    652 us
  parse       bench.c       :    151 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    125 us
  link        bench_o1      :    135 us
  link        bench_o1      :  54348 us

RCC -O2:
  preprocess  bench.c       :    654 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      9 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    239 us
  link        bench_o2      :    156 us
  link        bench_o2      :  54094 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 241167 us
  parse       sqlite3.c     :  59635 us
  typecheck   sqlite3.c     :  13868 us
  codegen     sqlite3.c     :  96496 us
  link        sqlite3.so    :  14590 us

RCC -O1:
  preprocess  sqlite3.c     : 259253 us
  parse       sqlite3.c     :  55864 us
  typecheck   sqlite3.c     :  12564 us
  opt         sqlite3.c     : 149852 us
  codegen     sqlite3.c     : 110533 us
  link        sqlite3.so    :  15303 us

RCC -O2:
  preprocess  sqlite3.c     : 203254 us
  parse       sqlite3.c     :  55801 us
  typecheck   sqlite3.c     :  12621 us
  opt         sqlite3.c     : 154783 us
  codegen     sqlite3.c     : 112112 us
  link        sqlite3.so    :  15946 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       754 ms |
| RCC -O1   |       662 ms |
| RCC -O2   |       648 ms |
| TCC       |       101 ms |
| GCC -O0   |       936 ms |
| GCC -O2   |      9123 ms |
| Clang -O0 |       926 ms |
| Clang -O2 |      8863 ms |
