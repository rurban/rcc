# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          122 |          848 |        970 |
| RCC -O1   |          106 |          889 |        995 |
| RCC -O2   |          148 |          753 |        901 |
| TCC       |           75 |          673 |        748 |
| GCC -O0   |          174 |          614 |        788 |
| GCC -O2   |          211 |          335 |        546 |
| Clang -O0 |          114 |          524 |        638 |
| Clang -O2 |          173 |          313 |        486 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1689 us
  parse       bench.c       :    741 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    459 us
  link        bench_rcc     :    215 us
  link        bench_rcc     : 122235 us

RCC -O1:
  preprocess  bench.c       :    695 us
  parse       bench.c       :    147 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    114 us
  link        bench_o1      :    372 us
  link        bench_o1      :  83195 us

RCC -O2:
  preprocess  bench.c       :    751 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    156 us
  link        bench_o2      :    231 us
  link        bench_o2      :  70114 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 489236 us
  parse       sqlite3.c     : 242409 us
  typecheck   sqlite3.c     :  25660 us
  codegen     sqlite3.c     : 315359 us
  link        sqlite3.so    :  28604 us

RCC -O1:
  preprocess  sqlite3.c     : 482324 us
  parse       sqlite3.c     : 108887 us
  typecheck   sqlite3.c     :  31941 us
  opt         sqlite3.c     :  32921 us
  codegen     sqlite3.c     : 145405 us
  link        sqlite3.so    :  17137 us

RCC -O2:
  preprocess  sqlite3.c     : 382154 us
  parse       sqlite3.c     :  84809 us
  typecheck   sqlite3.c     :  30710 us
  opt         sqlite3.c     : 299428 us
  codegen     sqlite3.c     : 190413 us
  link        sqlite3.so    :  18605 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1285 ms |
| RCC -O1   |      1288 ms |
| RCC -O2   |      1093 ms |
| TCC       |       131 ms |
| GCC -O0   |      1652 ms |
| GCC -O2   |     17373 ms |
| Clang -O0 |      2299 ms |
| Clang -O2 |     17734 ms |
