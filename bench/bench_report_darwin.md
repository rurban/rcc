# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           66 |          656 |        722 |
| RCC -O1   |           67 |          649 |        716 |
| RCC -O2   |           56 |          648 |        704 |
| TCC       |           42 |          581 |        623 |
| GCC -O0   |           71 |          498 |        569 |
| GCC -O2   |          103 |          297 |        400 |
| Clang -O0 |           66 |          489 |        555 |
| Clang -O2 |          106 |          290 |        396 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    660 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    138 us
  link        bench_rcc     :     71 us
  link        bench_rcc     :  48294 us

RCC -O1:
  preprocess  bench.c       :    751 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     31 us
  codegen     bench.c       :    125 us
  link        bench_o1      :     98 us
  link        bench_o1      :  50731 us

RCC -O2:
  preprocess  bench.c       :    593 us
  parse       bench.c       :    131 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    158 us
  link        bench_o2      :    119 us
  link        bench_o2      :  48912 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 242622 us
  parse       sqlite3.c     :  59803 us
  typecheck   sqlite3.c     :  21571 us
  codegen     sqlite3.c     : 126993 us
  link        sqlite3.so    :  30516 us

RCC -O1:
  preprocess  sqlite3.c     : 236847 us
  parse       sqlite3.c     :  53092 us
  typecheck   sqlite3.c     :  15072 us
  opt         sqlite3.c     : 153758 us
  codegen     sqlite3.c     : 109952 us
  link        sqlite3.so    :  15124 us

RCC -O2:
  preprocess  sqlite3.c     : 262816 us
  parse       sqlite3.c     :  52422 us
  typecheck   sqlite3.c     :  13997 us
  opt         sqlite3.c     : 154459 us
  codegen     sqlite3.c     : 124366 us
  link        sqlite3.so    :  15994 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       689 ms |
| RCC -O1   |       820 ms |
| RCC -O2   |       847 ms |
| TCC       |       107 ms |
| GCC -O0   |      1155 ms |
| GCC -O2   |     11108 ms |
| Clang -O0 |      1079 ms |
| Clang -O2 |     10651 ms |
