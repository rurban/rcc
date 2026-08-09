# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           48 |          584 |        632 |
| RCC -O1   |           49 |          582 |        631 |
| RCC -O2   |           49 |          585 |        634 |
| TCC       |           40 |          512 |        552 |
| GCC -O0   |           56 |          433 |        489 |
| GCC -O2   |           87 |          262 |        349 |
| Clang -O0 |           51 |          434 |        485 |
| Clang -O2 |           82 |          262 |        344 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    545 us
  parse       bench.c       :    114 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    110 us
  link        bench_rcc     :    107 us
  link        bench_rcc     :  41740 us

RCC -O1:
  preprocess  bench.c       :    529 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    117 us
  link        bench_o1      :    148 us
  link        bench_o1      :  41844 us

RCC -O2:
  preprocess  bench.c       :    522 us
  parse       bench.c       :    106 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    101 us
  link        bench_o2      :    170 us
  link        bench_o2      :  43266 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 216153 us
  parse       sqlite3.c     :  42395 us
  typecheck   sqlite3.c     :  11310 us
  codegen     sqlite3.c     :  85156 us
  link        sqlite3.so    :  13674 us

RCC -O1:
  preprocess  sqlite3.c     : 186935 us
  parse       sqlite3.c     :  40487 us
  typecheck   sqlite3.c     :  11331 us
  opt         sqlite3.c     :  17942 us
  codegen     sqlite3.c     :  83413 us
  link        sqlite3.so    :  13760 us

RCC -O2:
  preprocess  sqlite3.c     : 207090 us
  parse       sqlite3.c     :  45734 us
  typecheck   sqlite3.c     :  13940 us
  opt         sqlite3.c     : 136442 us
  codegen     sqlite3.c     : 101941 us
  link        sqlite3.so    :  14382 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       547 ms |
| RCC -O1   |       561 ms |
| RCC -O2   |       687 ms |
| TCC       |        92 ms |
| GCC -O0   |       913 ms |
| GCC -O2   |      9329 ms |
| Clang -O0 |       961 ms |
| Clang -O2 |      8601 ms |
