# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           94 |          688 |        782 |
| RCC -O1   |           89 |          705 |        794 |
| RCC -O2   |           63 |          727 |        790 |
| TCC       |           82 |          582 |        664 |
| GCC -O0   |           90 |          521 |        611 |
| GCC -O2   |          152 |          400 |        552 |
| Clang -O0 |           77 |          702 |        779 |
| Clang -O2 |          156 |          351 |        507 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    827 us
  parse       bench.c       :    170 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    144 us
  link        bench_rcc     :     65 us
  link        bench_rcc     :  59842 us

RCC -O1:
  preprocess  bench.c       :    773 us
  parse       bench.c       :    153 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    171 us
  link        bench_o1      :    137 us
  link        bench_o1      :  58642 us

RCC -O2:
  preprocess  bench.c       :    770 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    144 us
  link        bench_o2      :     85 us
  link        bench_o2      :  59738 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 267759 us
  parse       sqlite3.c     :  58585 us
  typecheck   sqlite3.c     :  33702 us
  codegen     sqlite3.c     : 124408 us
  link        sqlite3.so    :  17824 us

RCC -O1:
  preprocess  sqlite3.c     : 275950 us
  parse       sqlite3.c     :  60836 us
  typecheck   sqlite3.c     :  17208 us
  opt         sqlite3.c     :  21084 us
  codegen     sqlite3.c     : 115437 us
  link        sqlite3.so    :  17075 us

RCC -O2:
  preprocess  sqlite3.c     : 271404 us
  parse       sqlite3.c     :  57450 us
  typecheck   sqlite3.c     :  13700 us
  opt         sqlite3.c     : 187445 us
  codegen     sqlite3.c     : 137772 us
  link        sqlite3.so    :  18659 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1437 ms |
| RCC -O1   |      1252 ms |
| RCC -O2   |      1713 ms |
| TCC       |       217 ms |
| GCC -O0   |      2010 ms |
| GCC -O2   |     22615 ms |
| Clang -O0 |      2178 ms |
| Clang -O2 |     15276 ms |
