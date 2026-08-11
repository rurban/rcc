# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          103 |          709 |        812 |
| RCC -O1   |          106 |          687 |        793 |
| RCC -O2   |           83 |          671 |        754 |
| TCC       |           56 |          608 |        664 |
| GCC -O0   |          105 |          523 |        628 |
| GCC -O2   |          138 |          315 |        453 |
| Clang -O0 |           73 |          531 |        604 |
| Clang -O2 |          132 |          309 |        441 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1066 us
  parse       bench.c       :    205 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    157 us
  link        bench_rcc     :    435 us
  link        bench_rcc     :  75344 us

RCC -O1:
  preprocess  bench.c       :    725 us
  parse       bench.c       :    150 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    125 us
  link        bench_o1      :    557 us
  link        bench_o1      :  87488 us

RCC -O2:
  preprocess  bench.c       :   2042 us
  parse       bench.c       :    374 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    576 us
  link        bench_o2      :   1346 us
  link        bench_o2      :  68676 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 305455 us
  parse       sqlite3.c     :  77833 us
  typecheck   sqlite3.c     :  28502 us
  codegen     sqlite3.c     : 119957 us
  link        sqlite3.so    :  19411 us

RCC -O1:
  preprocess  sqlite3.c     : 382180 us
  parse       sqlite3.c     :  71940 us
  typecheck   sqlite3.c     :  17822 us
  opt         sqlite3.c     :  25786 us
  codegen     sqlite3.c     : 107472 us
  link        sqlite3.so    :  20989 us

RCC -O2:
  preprocess  sqlite3.c     : 263211 us
  parse       sqlite3.c     :  61650 us
  typecheck   sqlite3.c     :  17702 us
  opt         sqlite3.c     : 188237 us
  codegen     sqlite3.c     : 132126 us
  link        sqlite3.so    :  18660 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       831 ms |
| RCC -O1   |       740 ms |
| RCC -O2   |       895 ms |
| TCC       |       119 ms |
| GCC -O0   |      1291 ms |
| GCC -O2   |     13961 ms |
| Clang -O0 |      1512 ms |
| Clang -O2 |     15231 ms |
