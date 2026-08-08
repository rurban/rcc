# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           92 |          696 |        788 |
| RCC -O1   |           88 |          737 |        825 |
| RCC -O2   |           68 |          744 |        812 |
| TCC       |           68 |          572 |        640 |
| GCC -O0   |           82 |          603 |        685 |
| GCC -O2   |          218 |          340 |        558 |
| Clang -O0 |           93 |          557 |        650 |
| Clang -O2 |          115 |          330 |        445 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    812 us
  parse       bench.c       :    160 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    147 us
  link        bench_rcc     :    333 us
  link        bench_rcc     :  59814 us

RCC -O1:
  preprocess  bench.c       :    712 us
  parse       bench.c       :    121 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    127 us
  link        bench_o1      :    823 us
  link        bench_o1      :  75969 us

RCC -O2:
  preprocess  bench.c       :    765 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    124 us
  link        bench_o2      :    515 us
  link        bench_o2      :  71116 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 367554 us
  parse       sqlite3.c     :  61845 us
  typecheck   sqlite3.c     :  19442 us
  codegen     sqlite3.c     : 118820 us
  link        sqlite3.so    :  18132 us

RCC -O1:
  preprocess  sqlite3.c     : 272328 us
  parse       sqlite3.c     :  59940 us
  typecheck   sqlite3.c     :  14677 us
  opt         sqlite3.c     :  31915 us
  codegen     sqlite3.c     : 122341 us
  link        sqlite3.so    :  17810 us

RCC -O2:
  preprocess  sqlite3.c     : 245741 us
  parse       sqlite3.c     :  50427 us
  typecheck   sqlite3.c     :  13533 us
  opt         sqlite3.c     : 164130 us
  codegen     sqlite3.c     : 114598 us
  link        sqlite3.so    :  19118 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       961 ms |
| RCC -O1   |       663 ms |
| RCC -O2   |       796 ms |
| TCC       |       108 ms |
| GCC -O0   |      1306 ms |
| GCC -O2   |     13657 ms |
| Clang -O0 |      1218 ms |
| Clang -O2 |     11580 ms |
