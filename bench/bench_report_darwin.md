# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          113 |          802 |        915 |
| RCC -O1   |           87 |          774 |        861 |
| RCC -O2   |           83 |          819 |        902 |
| TCC       |           72 |          696 |        768 |
| GCC -O0   |           98 |          628 |        726 |
| GCC -O2   |          190 |          356 |        546 |
| Clang -O0 |           88 |          658 |        746 |
| Clang -O2 |          204 |          375 |        579 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1704 us
  parse       bench.c       :    358 us
  typecheck   bench.c       :     10 us
  codegen     bench.c       :    302 us
  link        bench_rcc     :    497 us
  link        bench_rcc     :  79480 us

RCC -O1:
  preprocess  bench.c       :    735 us
  parse       bench.c       :    171 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    119 us
  link        bench_o1      :    435 us
  link        bench_o1      :  66984 us

RCC -O2:
  preprocess  bench.c       :    664 us
  parse       bench.c       :    173 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    160 us
  link        bench_o2      :    486 us
  link        bench_o2      :  70428 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 327971 us
  parse       sqlite3.c     :  75648 us
  typecheck   sqlite3.c     :  20103 us
  codegen     sqlite3.c     : 119657 us
  link        sqlite3.so    :  18032 us

RCC -O1:
  preprocess  sqlite3.c     : 300688 us
  parse       sqlite3.c     :  65571 us
  typecheck   sqlite3.c     :  19005 us
  opt         sqlite3.c     : 211724 us
  codegen     sqlite3.c     : 177348 us
  link        sqlite3.so    :  24615 us

RCC -O2:
  preprocess  sqlite3.c     : 231004 us
  parse       sqlite3.c     : 121850 us
  typecheck   sqlite3.c     :  17061 us
  opt         sqlite3.c     : 248812 us
  codegen     sqlite3.c     : 167129 us
  link        sqlite3.so    :  17252 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1551 ms |
| RCC -O1   |      1470 ms |
| RCC -O2   |      1370 ms |
| TCC       |       228 ms |
| GCC -O0   |      2143 ms |
| GCC -O2   |     16492 ms |
| Clang -O0 |      1691 ms |
| Clang -O2 |     15979 ms |
