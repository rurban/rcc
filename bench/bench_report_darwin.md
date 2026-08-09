# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           89 |          606 |        695 |
| RCC -O1   |           55 |          612 |        667 |
| RCC -O2   |           54 |          585 |        639 |
| TCC       |           82 |          563 |        645 |
| GCC -O0   |           80 |          463 |        543 |
| GCC -O2   |          111 |          263 |        374 |
| Clang -O0 |           55 |          433 |        488 |
| Clang -O2 |           82 |          263 |        345 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    722 us
  parse       bench.c       :    184 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    126 us
  link        bench_rcc     :    198 us
  link        bench_rcc     :  67759 us

RCC -O1:
  preprocess  bench.c       :    672 us
  parse       bench.c       :    133 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    134 us
  link        bench_o1      :    156 us
  link        bench_o1      :  55563 us

RCC -O2:
  preprocess  bench.c       :    578 us
  parse       bench.c       :    141 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    116 us
  link        bench_o2      :     98 us
  link        bench_o2      :  46876 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 270154 us
  parse       sqlite3.c     : 138919 us
  typecheck   sqlite3.c     :  26313 us
  codegen     sqlite3.c     : 119417 us
  link        sqlite3.so    :  14410 us

RCC -O1:
  preprocess  sqlite3.c     : 258049 us
  parse       sqlite3.c     :  44691 us
  typecheck   sqlite3.c     :  12497 us
  opt         sqlite3.c     :  19220 us
  codegen     sqlite3.c     :  89664 us
  link        sqlite3.so    :  14396 us

RCC -O2:
  preprocess  sqlite3.c     : 213139 us
  parse       sqlite3.c     :  52674 us
  typecheck   sqlite3.c     :  13640 us
  opt         sqlite3.c     : 156642 us
  codegen     sqlite3.c     : 103401 us
  link        sqlite3.so    :  17246 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       644 ms |
| RCC -O1   |       572 ms |
| RCC -O2   |       722 ms |
| TCC       |       102 ms |
| GCC -O0   |      1023 ms |
| GCC -O2   |     10144 ms |
| Clang -O0 |       992 ms |
| Clang -O2 |      9039 ms |
