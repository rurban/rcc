# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           68 |          635 |        703 |
| RCC -O1   |           71 |          637 |        708 |
| RCC -O2   |           87 |          632 |        719 |
| TCC       |           57 |          561 |        618 |
| GCC -O0   |           84 |          471 |        555 |
| GCC -O2   |          119 |          308 |        427 |
| Clang -O0 |          104 |          470 |        574 |
| Clang -O2 |          113 |          286 |        399 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    697 us
  parse       bench.c       :    125 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    133 us
  link        bench_rcc     :    200 us
  link        bench_rcc     :  71681 us

RCC -O1:
  preprocess  bench.c       :    655 us
  parse       bench.c       :    122 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    119 us
  link        bench_o1      :    214 us
  link        bench_o1      :  62236 us

RCC -O2:
  preprocess  bench.c       :    718 us
  parse       bench.c       :    135 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    127 us
  link        bench_o2      :    238 us
  link        bench_o2      :  62990 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 245675 us
  parse       sqlite3.c     :  50915 us
  typecheck   sqlite3.c     :  14478 us
  codegen     sqlite3.c     : 104897 us
  link        sqlite3.so    :  15334 us

RCC -O1:
  preprocess  sqlite3.c     : 239251 us
  parse       sqlite3.c     :  51593 us
  typecheck   sqlite3.c     :  14623 us
  opt         sqlite3.c     : 142346 us
  codegen     sqlite3.c     : 109608 us
  link        sqlite3.so    :  17434 us

RCC -O2:
  preprocess  sqlite3.c     : 241041 us
  parse       sqlite3.c     :  50648 us
  typecheck   sqlite3.c     :  14604 us
  opt         sqlite3.c     : 146774 us
  codegen     sqlite3.c     : 107178 us
  link        sqlite3.so    :  16626 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       662 ms |
| RCC -O1   |       797 ms |
| RCC -O2   |       786 ms |
| TCC       |       106 ms |
| GCC -O0   |      1097 ms |
| GCC -O2   |     12041 ms |
| Clang -O0 |      1113 ms |
| Clang -O2 |     11298 ms |
