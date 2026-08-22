# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           47 |          621 |        668 |
| RCC -O1   |           49 |          619 |        668 |
| RCC -O2   |           50 |          616 |        666 |
| TCC       |           43 |          528 |        571 |
| GCC -O0   |           57 |          445 |        502 |
| GCC -O2   |           87 |          269 |        356 |
| Clang -O0 |           54 |          450 |        504 |
| Clang -O2 |           85 |          269 |        354 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    583 us
  parse       bench.c       :    115 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    110 us
  link        bench_rcc     :    112 us
  link        bench_rcc     :  43356 us

RCC -O1:
  preprocess  bench.c       :    576 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    146 us
  link        bench_o1      :    107 us
  link        bench_o1      :  48616 us

RCC -O2:
  preprocess  bench.c       :    544 us
  parse       bench.c       :    130 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    121 us
  link        bench_o2      :    152 us
  link        bench_o2      :  42772 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 185061 us
  parse       sqlite3.c     :  44561 us
  typecheck   sqlite3.c     :  11740 us
  codegen     sqlite3.c     :  87398 us
  link        sqlite3.so    :  15047 us

RCC -O1:
  preprocess  sqlite3.c     : 184696 us
  parse       sqlite3.c     :  45222 us
  typecheck   sqlite3.c     :  11663 us
  opt         sqlite3.c     : 127474 us
  codegen     sqlite3.c     : 101443 us
  link        sqlite3.so    :  15381 us

RCC -O2:
  preprocess  sqlite3.c     : 211138 us
  parse       sqlite3.c     :  52873 us
  typecheck   sqlite3.c     :  15248 us
  opt         sqlite3.c     : 130561 us
  codegen     sqlite3.c     :  90966 us
  link        sqlite3.so    :  15811 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       537 ms |
| RCC -O1   |       651 ms |
| RCC -O2   |       660 ms |
| TCC       |        90 ms |
| GCC -O0   |       950 ms |
| GCC -O2   |      9322 ms |
| Clang -O0 |       975 ms |
| Clang -O2 |      9457 ms |
