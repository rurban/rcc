# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           86 |          715 |        801 |
| RCC -O1   |           69 |          766 |        835 |
| RCC -O2   |           95 |          667 |        762 |
| TCC       |           65 |          587 |        652 |
| GCC -O0   |           87 |          478 |        565 |
| GCC -O2   |          140 |          311 |        451 |
| Clang -O0 |           60 |          527 |        587 |
| Clang -O2 |          124 |          329 |        453 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    915 us
  parse       bench.c       :    228 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    207 us
  link        bench_rcc     :    355 us
  link        bench_rcc     :  56246 us

RCC -O1:
  preprocess  bench.c       :    709 us
  parse       bench.c       :    179 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    153 us
  link        bench_o1      :    125 us
  link        bench_o1      :  52882 us

RCC -O2:
  preprocess  bench.c       :    926 us
  parse       bench.c       :    197 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    136 us
  link        bench_o2      :    124 us
  link        bench_o2      :  68116 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 356389 us
  parse       sqlite3.c     :  85821 us
  typecheck   sqlite3.c     :  21413 us
  codegen     sqlite3.c     : 113846 us
  link        sqlite3.so    :  15637 us

RCC -O1:
  preprocess  sqlite3.c     : 343568 us
  parse       sqlite3.c     :  75889 us
  typecheck   sqlite3.c     :  13366 us
  opt         sqlite3.c     :  20396 us
  codegen     sqlite3.c     : 128597 us
  link        sqlite3.so    :  19151 us

RCC -O2:
  preprocess  sqlite3.c     : 285946 us
  parse       sqlite3.c     :  55994 us
  typecheck   sqlite3.c     :  13758 us
  opt         sqlite3.c     : 146576 us
  codegen     sqlite3.c     : 112589 us
  link        sqlite3.so    :  18272 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       833 ms |
| RCC -O1   |       780 ms |
| RCC -O2   |       959 ms |
| TCC       |       117 ms |
| GCC -O0   |      1234 ms |
| GCC -O2   |     12268 ms |
| Clang -O0 |      1313 ms |
| Clang -O2 |     11707 ms |
