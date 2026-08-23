# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           69 |          644 |        713 |
| RCC -O1   |           71 |          655 |        726 |
| RCC -O2   |           82 |          664 |        746 |
| TCC       |           56 |          563 |        619 |
| GCC -O0   |           80 |          486 |        566 |
| GCC -O2   |          152 |          294 |        446 |
| Clang -O0 |           70 |          480 |        550 |
| Clang -O2 |          111 |          288 |        399 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    855 us
  parse       bench.c       :    166 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    268 us
  link        bench_rcc     :    225 us
  link        bench_rcc     :  95856 us

RCC -O1:
  preprocess  bench.c       :    930 us
  parse       bench.c       :    185 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    256 us
  link        bench_o1      :    355 us
  link        bench_o1      :  98212 us

RCC -O2:
  preprocess  bench.c       :    889 us
  parse       bench.c       :    292 us
  typecheck   bench.c       :      9 us
  opt         bench.c       :     44 us
  codegen     bench.c       :    196 us
  link        bench_o2      :    251 us
  link        bench_o2      :  76847 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 282972 us
  parse       sqlite3.c     :  67220 us
  typecheck   sqlite3.c     :  15483 us
  codegen     sqlite3.c     : 158664 us
  link        sqlite3.so    :  20385 us

RCC -O1:
  preprocess  sqlite3.c     : 277185 us
  parse       sqlite3.c     :  54879 us
  typecheck   sqlite3.c     :  14221 us
  opt         sqlite3.c     : 141665 us
  codegen     sqlite3.c     : 104060 us
  link        sqlite3.so    :  15152 us

RCC -O2:
  preprocess  sqlite3.c     : 246602 us
  parse       sqlite3.c     :  56068 us
  typecheck   sqlite3.c     :  14068 us
  opt         sqlite3.c     : 155863 us
  codegen     sqlite3.c     : 117804 us
  link        sqlite3.so    :  17692 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       622 ms |
| RCC -O1   |       751 ms |
| RCC -O2   |       819 ms |
| TCC       |       126 ms |
| GCC -O0   |      1125 ms |
| GCC -O2   |     12077 ms |
| Clang -O0 |      1167 ms |
| Clang -O2 |     11614 ms |
