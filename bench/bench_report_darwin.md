# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          184 |          909 |       1093 |
| RCC -O1   |          202 |         1159 |       1361 |
| RCC -O2   |          165 |          910 |       1075 |
| TCC       |          107 |          802 |        909 |
| GCC -O0   |          230 |          647 |        877 |
| GCC -O2   |          190 |          371 |        561 |
| Clang -O0 |          104 |          706 |        810 |
| Clang -O2 |          182 |          403 |        585 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    925 us
  parse       bench.c       :    361 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    199 us
  link        bench_rcc     :    343 us
  link        bench_rcc     :  95395 us

RCC -O1:
  preprocess  bench.c       :   1520 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    149 us
  link        bench_o1      :    505 us
  link        bench_o1      : 109905 us

RCC -O2:
  preprocess  bench.c       :   1509 us
  parse       bench.c       :    326 us
  typecheck   bench.c       :     11 us
  opt         bench.c       :     48 us
  codegen     bench.c       :    299 us
  link        bench_o2      :    691 us
  link        bench_o2      : 112239 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 462317 us
  parse       sqlite3.c     : 223853 us
  typecheck   sqlite3.c     :  27233 us
  codegen     sqlite3.c     : 256068 us
  link        sqlite3.so    :  41808 us

RCC -O1:
  preprocess  sqlite3.c     : 570741 us
  parse       sqlite3.c     :  88111 us
  typecheck   sqlite3.c     :  23506 us
  opt         sqlite3.c     : 439038 us
  codegen     sqlite3.c     : 250770 us
  link        sqlite3.so    :  35450 us

RCC -O2:
  preprocess  sqlite3.c     : 656400 us
  parse       sqlite3.c     : 241913 us
  typecheck   sqlite3.c     :  46929 us
  opt         sqlite3.c     : 788109 us
  codegen     sqlite3.c     : 315907 us
  link        sqlite3.so    :  23609 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1568 ms |
| RCC -O1   |      1716 ms |
| RCC -O2   |      1636 ms |
| TCC       |       176 ms |
| GCC -O0   |      2444 ms |
| GCC -O2   |     21945 ms |
| Clang -O0 |      2038 ms |
| Clang -O2 |     23856 ms |
