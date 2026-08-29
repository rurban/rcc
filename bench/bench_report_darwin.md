# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           71 |          688 |        759 |
| RCC -O1   |           70 |          690 |        760 |
| RCC -O2   |           70 |          687 |        757 |
| TCC       |           82 |          622 |        704 |
| GCC -O0   |          170 |          593 |        763 |
| GCC -O2   |          194 |          364 |        558 |
| Clang -O0 |          119 |          589 |        708 |
| Clang -O2 |          163 |          320 |        483 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    676 us
  parse       bench.c       :    125 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    131 us
  link        bench_rcc     :     78 us
  link        bench_rcc     :  51839 us

RCC -O1:
  preprocess  bench.c       :    647 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    139 us
  link        bench_o1      :    101 us
  link        bench_o1      :  52236 us

RCC -O2:
  preprocess  bench.c       :    649 us
  parse       bench.c       :    157 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    141 us
  link        bench_o2      :     88 us
  link        bench_o2      :  51075 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 226807 us
  parse       sqlite3.c     :  52559 us
  typecheck   sqlite3.c     :  11574 us
  codegen     sqlite3.c     : 111708 us
  link        sqlite3.so    :  16655 us

RCC -O1:
  preprocess  sqlite3.c     : 191310 us
  parse       sqlite3.c     :  49312 us
  typecheck   sqlite3.c     :  10805 us
  opt         sqlite3.c     : 137216 us
  codegen     sqlite3.c     : 102435 us
  link        sqlite3.so    :  14950 us

RCC -O2:
  preprocess  sqlite3.c     : 191945 us
  parse       sqlite3.c     :  50003 us
  typecheck   sqlite3.c     :  10705 us
  opt         sqlite3.c     : 142870 us
  codegen     sqlite3.c     : 122095 us
  link        sqlite3.so    :  16721 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       796 ms |
| RCC -O1   |      1226 ms |
| RCC -O2   |      1121 ms |
| TCC       |       155 ms |
| GCC -O0   |      1349 ms |
| GCC -O2   |     11108 ms |
| Clang -O0 |      1388 ms |
| Clang -O2 |     12197 ms |
