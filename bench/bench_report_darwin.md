# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           78 |          722 |        800 |
| RCC -O1   |           71 |          717 |        788 |
| RCC -O2   |           75 |          738 |        813 |
| TCC       |           67 |          626 |        693 |
| GCC -O0   |          104 |          539 |        643 |
| GCC -O2   |          156 |          319 |        475 |
| Clang -O0 |           60 |          521 |        581 |
| Clang -O2 |          121 |          321 |        442 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1140 us
  parse       bench.c       :    210 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    148 us
  link        bench_rcc     :    276 us
  link        bench_rcc     :  78212 us

RCC -O1:
  preprocess  bench.c       :   1099 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    159 us
  link        bench_o1      :    360 us
  link        bench_o1      :  67466 us

RCC -O2:
  preprocess  bench.c       :   1009 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    139 us
  link        bench_o2      :    365 us
  link        bench_o2      :  71657 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 281615 us
  parse       sqlite3.c     :  70526 us
  typecheck   sqlite3.c     :  16020 us
  codegen     sqlite3.c     : 138115 us
  link        sqlite3.so    :  20205 us

RCC -O1:
  preprocess  sqlite3.c     : 298510 us
  parse       sqlite3.c     :  61676 us
  typecheck   sqlite3.c     :  15543 us
  opt         sqlite3.c     : 172138 us
  codegen     sqlite3.c     : 170175 us
  link        sqlite3.so    :  20069 us

RCC -O2:
  preprocess  sqlite3.c     : 355199 us
  parse       sqlite3.c     :  73915 us
  typecheck   sqlite3.c     :  16408 us
  opt         sqlite3.c     : 242750 us
  codegen     sqlite3.c     : 121172 us
  link        sqlite3.so    :  17723 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       698 ms |
| RCC -O1   |       877 ms |
| RCC -O2   |       906 ms |
| TCC       |       112 ms |
| GCC -O0   |      1527 ms |
| GCC -O2   |     13499 ms |
| Clang -O0 |      1465 ms |
| Clang -O2 |     12870 ms |
