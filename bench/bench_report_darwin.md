# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           69 |          661 |        730 |
| RCC -O1   |           79 |          699 |        778 |
| RCC -O2   |           57 |          649 |        706 |
| TCC       |           79 |          581 |        660 |
| GCC -O0   |          146 |          470 |        616 |
| GCC -O2   |          121 |          286 |        407 |
| Clang -O0 |           92 |          473 |        565 |
| Clang -O2 |          122 |          321 |        443 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    679 us
  parse       bench.c       :    131 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    127 us
  link        bench_rcc     :     89 us
  link        bench_rcc     :  57323 us

RCC -O1:
  preprocess  bench.c       :    705 us
  parse       bench.c       :    200 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    144 us
  link        bench_o1      :    173 us
  link        bench_o1      :  63339 us

RCC -O2:
  preprocess  bench.c       :    795 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    130 us
  link        bench_o2      :    470 us
  link        bench_o2      :  66756 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 407273 us
  parse       sqlite3.c     : 134427 us
  typecheck   sqlite3.c     :  25467 us
  codegen     sqlite3.c     : 175893 us
  link        sqlite3.so    :  16051 us

RCC -O1:
  preprocess  sqlite3.c     : 329417 us
  parse       sqlite3.c     :  71184 us
  typecheck   sqlite3.c     :  21781 us
  opt         sqlite3.c     :  25494 us
  codegen     sqlite3.c     : 178662 us
  link        sqlite3.so    :  17991 us

RCC -O2:
  preprocess  sqlite3.c     : 340594 us
  parse       sqlite3.c     :  55652 us
  typecheck   sqlite3.c     :  14648 us
  opt         sqlite3.c     : 188564 us
  codegen     sqlite3.c     : 148964 us
  link        sqlite3.so    :  14685 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       906 ms |
| RCC -O1   |      1008 ms |
| RCC -O2   |       877 ms |
| TCC       |       387 ms |
| GCC -O0   |      1637 ms |
| GCC -O2   |     10970 ms |
| Clang -O0 |      1544 ms |
| Clang -O2 |     17757 ms |
