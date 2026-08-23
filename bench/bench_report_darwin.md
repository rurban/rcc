# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           65 |          646 |        711 |
| RCC -O1   |           57 |          661 |        718 |
| RCC -O2   |           55 |          636 |        691 |
| TCC       |           40 |          584 |        624 |
| GCC -O0   |           68 |          460 |        528 |
| GCC -O2   |           96 |          294 |        390 |
| Clang -O0 |           64 |          469 |        533 |
| Clang -O2 |           99 |          274 |        373 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    638 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    120 us
  link        bench_rcc     :     65 us
  link        bench_rcc     :  45310 us

RCC -O1:
  preprocess  bench.c       :    689 us
  parse       bench.c       :    125 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    127 us
  link        bench_o1      :    112 us
  link        bench_o1      :  54707 us

RCC -O2:
  preprocess  bench.c       :    690 us
  parse       bench.c       :    141 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    130 us
  link        bench_o2      :    130 us
  link        bench_o2      :  56216 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 252213 us
  parse       sqlite3.c     :  56006 us
  typecheck   sqlite3.c     :  13995 us
  codegen     sqlite3.c     :  95231 us
  link        sqlite3.so    :  14238 us

RCC -O1:
  preprocess  sqlite3.c     : 185747 us
  parse       sqlite3.c     :  50321 us
  typecheck   sqlite3.c     :  13950 us
  opt         sqlite3.c     : 125558 us
  codegen     sqlite3.c     : 105757 us
  link        sqlite3.so    :  15602 us

RCC -O2:
  preprocess  sqlite3.c     : 218463 us
  parse       sqlite3.c     :  51167 us
  typecheck   sqlite3.c     :  15023 us
  opt         sqlite3.c     : 131345 us
  codegen     sqlite3.c     : 103627 us
  link        sqlite3.so    :  16161 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       794 ms |
| RCC -O1   |       688 ms |
| RCC -O2   |       708 ms |
| TCC       |        98 ms |
| GCC -O0   |       994 ms |
| GCC -O2   |     11104 ms |
| Clang -O0 |      1551 ms |
| Clang -O2 |     12900 ms |
