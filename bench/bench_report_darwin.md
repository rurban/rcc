# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          215 |          793 |       1008 |
| RCC -O1   |          112 |          737 |        849 |
| RCC -O2   |           81 |          764 |        845 |
| TCC       |           71 |          714 |        785 |
| GCC -O0   |          128 |          566 |        694 |
| GCC -O2   |          135 |          360 |        495 |
| Clang -O0 |           96 |          549 |        645 |
| Clang -O2 |          131 |          292 |        423 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          196 |         5741 |       5937 |
| RCC -O1   |          238 |         5567 |       5805 |
| RCC -O2   |          448 |         4756 |       5204 |
| TCC       |          213 |         4364 |       4577 |
| GCC -O0   |          673 |         3527 |       4200 |
| GCC -O2   |          932 |         2054 |       2986 |
| Clang -O0 |          581 |         3505 |       4086 |
| Clang -O2 |          896 |         2033 |       2929 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    879 us
  parse       bench.c       :    179 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    173 us
  link        bench_rcc     :     84 us
  link        bench_rcc     :  65924 us

RCC -O1:
  preprocess  bench.c       :    665 us
  parse       bench.c       :    192 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    172 us
  link        bench_o1      :    100 us
  link        bench_o1      :  60589 us

RCC -O2:
  preprocess  bench.c       :    641 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    154 us
  link        bench_o2      :    192 us
  link        bench_o2      :  63075 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 297026 us
  parse       sqlite3.c     :  70273 us
  typecheck   sqlite3.c     :  13436 us
  codegen     sqlite3.c     : 303650 us
  link        sqlite3.so    :  24078 us

RCC -O1:
  preprocess  sqlite3.c     : 292731 us
  parse       sqlite3.c     : 104200 us
  typecheck   sqlite3.c     :  15995 us
  opt         sqlite3.c     : 238058 us
  codegen     sqlite3.c     : 212641 us
  link        sqlite3.so    :  21449 us

RCC -O2:
  preprocess  sqlite3.c     : 367809 us
  parse       sqlite3.c     : 132014 us
  typecheck   sqlite3.c     :  17575 us
  opt         sqlite3.c     : 674328 us
  codegen     sqlite3.c     : 194459 us
  link        sqlite3.so    :  18706 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1153 ms |
| RCC -O1   |       800 ms |
| RCC -O2   |       764 ms |
| TCC       |       122 ms |
| GCC -O0   |      1132 ms |
| GCC -O2   |     10226 ms |
| Clang -O0 |      1122 ms |
| Clang -O2 |     11645 ms |
