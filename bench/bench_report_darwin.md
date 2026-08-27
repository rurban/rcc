# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          180 |          944 |       1124 |
| RCC -O1   |          115 |          868 |        983 |
| RCC -O2   |          125 |          859 |        984 |
| TCC       |           80 |          763 |        843 |
| GCC -O0   |          113 |          650 |        763 |
| GCC -O2   |          161 |          359 |        520 |
| Clang -O0 |           94 |          636 |        730 |
| Clang -O2 |          222 |          380 |        602 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1260 us
  parse       bench.c       :    339 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    291 us
  link        bench_rcc     :    226 us
  link        bench_rcc     :  79382 us

RCC -O1:
  preprocess  bench.c       :    742 us
  parse       bench.c       :    431 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     65 us
  codegen     bench.c       :    180 us
  link        bench_o1      :    292 us
  link        bench_o1      :  79745 us

RCC -O2:
  preprocess  bench.c       :    690 us
  parse       bench.c       :    153 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    176 us
  link        bench_o2      :    198 us
  link        bench_o2      :  82342 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 411305 us
  parse       sqlite3.c     :  75999 us
  typecheck   sqlite3.c     :  51846 us
  codegen     sqlite3.c     : 224586 us
  link        sqlite3.so    :  26133 us

RCC -O1:
  preprocess  sqlite3.c     : 420472 us
  parse       sqlite3.c     :  76452 us
  typecheck   sqlite3.c     :  16630 us
  opt         sqlite3.c     : 313695 us
  codegen     sqlite3.c     : 284365 us
  link        sqlite3.so    :  22281 us

RCC -O2:
  preprocess  sqlite3.c     : 896855 us
  parse       sqlite3.c     : 187279 us
  typecheck   sqlite3.c     :  33802 us
  opt         sqlite3.c     : 347633 us
  codegen     sqlite3.c     : 207235 us
  link        sqlite3.so    :  19173 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1532 ms |
| RCC -O1   |      1520 ms |
| RCC -O2   |      1333 ms |
| TCC       |       166 ms |
| GCC -O0   |      1816 ms |
| GCC -O2   |     16929 ms |
| Clang -O0 |      1980 ms |
| Clang -O2 |     17626 ms |
