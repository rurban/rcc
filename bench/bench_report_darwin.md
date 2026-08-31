# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           75 |          649 |        724 |
| RCC -O1   |           55 |          667 |        722 |
| RCC -O2   |           65 |          649 |        714 |
| TCC       |           65 |          560 |        625 |
| GCC -O0   |           70 |          472 |        542 |
| GCC -O2   |          136 |          289 |        425 |
| Clang -O0 |           55 |          500 |        555 |
| Clang -O2 |           94 |          290 |        384 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1019 us
  parse       bench.c       :    281 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    164 us
  link        bench_rcc     :    233 us
  link        bench_rcc     :  82637 us

RCC -O1:
  preprocess  bench.c       :    644 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    147 us
  link        bench_o1      :     96 us
  link        bench_o1      :  52502 us

RCC -O2:
  preprocess  bench.c       :    724 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    149 us
  link        bench_o2      :    128 us
  link        bench_o2      :  62619 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 254577 us
  parse       sqlite3.c     :  70279 us
  typecheck   sqlite3.c     :  12506 us
  codegen     sqlite3.c     : 169218 us
  link        sqlite3.so    :  19387 us

RCC -O1:
  preprocess  sqlite3.c     : 243157 us
  parse       sqlite3.c     :  58314 us
  typecheck   sqlite3.c     :  12217 us
  opt         sqlite3.c     : 142674 us
  codegen     sqlite3.c     : 112178 us
  link        sqlite3.so    :  19696 us

RCC -O2:
  preprocess  sqlite3.c     : 291388 us
  parse       sqlite3.c     :  65045 us
  typecheck   sqlite3.c     :  13591 us
  opt         sqlite3.c     : 191684 us
  codegen     sqlite3.c     : 103009 us
  link        sqlite3.so    :  19444 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       750 ms |
| RCC -O1   |       857 ms |
| RCC -O2   |       766 ms |
| TCC       |       129 ms |
| GCC -O0   |      1285 ms |
| GCC -O2   |     10931 ms |
| Clang -O0 |      1312 ms |
| Clang -O2 |     12992 ms |
