# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           97 |          788 |        885 |
| RCC -O1   |          131 |          789 |        920 |
| RCC -O2   |           89 |          809 |        898 |
| TCC       |           80 |          699 |        779 |
| GCC -O0   |          140 |          594 |        734 |
| GCC -O2   |          175 |          350 |        525 |
| Clang -O0 |           89 |          522 |        611 |
| Clang -O2 |          144 |          348 |        492 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   3117 us
  parse       bench.c       :    491 us
  typecheck   bench.c       :      8 us
  codegen     bench.c       :    411 us
  link        bench_rcc     :    239 us
  link        bench_rcc     :  87215 us

RCC -O1:
  preprocess  bench.c       :   1611 us
  parse       bench.c       :    316 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     51 us
  codegen     bench.c       :    274 us
  link        bench_o1      :   6646 us
  link        bench_o1      : 104901 us

RCC -O2:
  preprocess  bench.c       :   1686 us
  parse       bench.c       :    319 us
  typecheck   bench.c       :     11 us
  opt         bench.c       :     50 us
  codegen     bench.c       :    276 us
  link        bench_o2      :    255 us
  link        bench_o2      :  81212 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 364460 us
  parse       sqlite3.c     : 208798 us
  typecheck   sqlite3.c     :  25882 us
  codegen     sqlite3.c     : 225458 us
  link        sqlite3.so    :  17083 us

RCC -O1:
  preprocess  sqlite3.c     : 448862 us
  parse       sqlite3.c     : 125746 us
  typecheck   sqlite3.c     :  25442 us
  opt         sqlite3.c     : 288969 us
  codegen     sqlite3.c     : 165498 us
  link        sqlite3.so    :  22303 us

RCC -O2:
  preprocess  sqlite3.c     : 359385 us
  parse       sqlite3.c     :  72586 us
  typecheck   sqlite3.c     :  22969 us
  opt         sqlite3.c     : 234689 us
  codegen     sqlite3.c     : 195515 us
  link        sqlite3.so    :  36944 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1406 ms |
| RCC -O1   |      1273 ms |
| RCC -O2   |      1183 ms |
| TCC       |       216 ms |
| GCC -O0   |      1860 ms |
| GCC -O2   |     14736 ms |
| Clang -O0 |      2077 ms |
| Clang -O2 |     18717 ms |
