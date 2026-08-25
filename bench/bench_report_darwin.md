# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           57 |          640 |        697 |
| RCC -O1   |           79 |          631 |        710 |
| RCC -O2   |           52 |          672 |        724 |
| TCC       |           68 |          543 |        611 |
| GCC -O0   |           89 |          466 |        555 |
| GCC -O2   |          139 |          275 |        414 |
| Clang -O0 |           55 |          454 |        509 |
| Clang -O2 |           88 |          275 |        363 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1367 us
  parse       bench.c       :    185 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    172 us
  link        bench_rcc     :     81 us
  link        bench_rcc     :  52683 us

RCC -O1:
  preprocess  bench.c       :    723 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    156 us
  link        bench_o1      :    117 us
  link        bench_o1      :  49412 us

RCC -O2:
  preprocess  bench.c       :    681 us
  parse       bench.c       :    147 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    143 us
  link        bench_o2      :     79 us
  link        bench_o2      :  46260 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 261923 us
  parse       sqlite3.c     :  60244 us
  typecheck   sqlite3.c     :  12324 us
  codegen     sqlite3.c     : 137856 us
  link        sqlite3.so    :  16035 us

RCC -O1:
  preprocess  sqlite3.c     : 233265 us
  parse       sqlite3.c     :  57194 us
  typecheck   sqlite3.c     :  16630 us
  opt         sqlite3.c     : 175420 us
  codegen     sqlite3.c     : 107050 us
  link        sqlite3.so    :  15016 us

RCC -O2:
  preprocess  sqlite3.c     : 223831 us
  parse       sqlite3.c     :  62859 us
  typecheck   sqlite3.c     :  14619 us
  opt         sqlite3.c     : 208565 us
  codegen     sqlite3.c     :  96261 us
  link        sqlite3.so    :  15423 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       620 ms |
| RCC -O1   |       695 ms |
| RCC -O2   |       701 ms |
| TCC       |        94 ms |
| GCC -O0   |       995 ms |
| GCC -O2   |     10754 ms |
| Clang -O0 |      1016 ms |
| Clang -O2 |      9506 ms |
