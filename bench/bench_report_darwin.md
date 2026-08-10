# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          191 |          874 |       1065 |
| RCC -O1   |          106 |          827 |        933 |
| RCC -O2   |          171 |          853 |       1024 |
| TCC       |          103 |          695 |        798 |
| GCC -O0   |          106 |          660 |        766 |
| GCC -O2   |          200 |          364 |        564 |
| Clang -O0 |          139 |          748 |        887 |
| Clang -O2 |          202 |          373 |        575 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    864 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    142 us
  link        bench_rcc     :    253 us
  link        bench_rcc     :  88000 us

RCC -O1:
  preprocess  bench.c       :    812 us
  parse       bench.c       :    167 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    146 us
  link        bench_o1      :    202 us
  link        bench_o1      :  84309 us

RCC -O2:
  preprocess  bench.c       :   1602 us
  parse       bench.c       :    220 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    184 us
  link        bench_o2      :    300 us
  link        bench_o2      :  83307 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 595658 us
  parse       sqlite3.c     : 110442 us
  typecheck   sqlite3.c     :  17466 us
  codegen     sqlite3.c     : 198384 us
  link        sqlite3.so    :  23077 us

RCC -O1:
  preprocess  sqlite3.c     : 534510 us
  parse       sqlite3.c     :  88352 us
  typecheck   sqlite3.c     :  22217 us
  opt         sqlite3.c     :  30306 us
  codegen     sqlite3.c     : 210071 us
  link        sqlite3.so    :  24712 us

RCC -O2:
  preprocess  sqlite3.c     : 481166 us
  parse       sqlite3.c     :  92643 us
  typecheck   sqlite3.c     :  17628 us
  opt         sqlite3.c     : 315920 us
  codegen     sqlite3.c     : 214168 us
  link        sqlite3.so    :  42177 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1818 ms |
| RCC -O1   |      1121 ms |
| RCC -O2   |      1342 ms |
| TCC       |       210 ms |
| GCC -O0   |      2127 ms |
| GCC -O2   |     17285 ms |
| Clang -O0 |      1513 ms |
| Clang -O2 |     15557 ms |
