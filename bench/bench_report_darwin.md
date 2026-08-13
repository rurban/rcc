# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          130 |          727 |        857 |
| RCC -O1   |           88 |          703 |        791 |
| RCC -O2   |           74 |          666 |        740 |
| TCC       |           49 |          583 |        632 |
| GCC -O0   |           86 |          489 |        575 |
| GCC -O2   |          108 |          303 |        411 |
| Clang -O0 |           62 |          498 |        560 |
| Clang -O2 |          124 |          298 |        422 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1071 us
  parse       bench.c       :    180 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    252 us
  link        bench_rcc     :    232 us
  link        bench_rcc     :  66138 us

RCC -O1:
  preprocess  bench.c       :   2820 us
  parse       bench.c       :    247 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    121 us
  link        bench_o1      :    204 us
  link        bench_o1      :  73604 us

RCC -O2:
  preprocess  bench.c       :    659 us
  parse       bench.c       :    132 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    126 us
  link        bench_o2      :    170 us
  link        bench_o2      :  77542 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 451445 us
  parse       sqlite3.c     : 176177 us
  typecheck   sqlite3.c     :  25665 us
  codegen     sqlite3.c     : 209992 us
  link        sqlite3.so    :  48599 us

RCC -O1:
  preprocess  sqlite3.c     : 487921 us
  parse       sqlite3.c     :  68913 us
  typecheck   sqlite3.c     :  21149 us
  opt         sqlite3.c     : 172406 us
  codegen     sqlite3.c     : 139772 us
  link        sqlite3.so    :  19960 us

RCC -O2:
  preprocess  sqlite3.c     : 292597 us
  parse       sqlite3.c     :  58353 us
  typecheck   sqlite3.c     :  16122 us
  opt         sqlite3.c     : 169226 us
  codegen     sqlite3.c     : 136894 us
  link        sqlite3.so    :  20680 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       999 ms |
| RCC -O1   |       995 ms |
| RCC -O2   |       961 ms |
| TCC       |       188 ms |
| GCC -O0   |      1099 ms |
| GCC -O2   |     11697 ms |
| Clang -O0 |      1441 ms |
| Clang -O2 |     10208 ms |
