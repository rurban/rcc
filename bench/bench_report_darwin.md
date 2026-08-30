# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           55 |          637 |        692 |
| RCC -O1   |           53 |          645 |        698 |
| RCC -O2   |           54 |          646 |        700 |
| TCC       |           41 |          555 |        596 |
| GCC -O0   |           64 |          469 |        533 |
| GCC -O2   |           94 |          283 |        377 |
| Clang -O0 |           56 |          468 |        524 |
| Clang -O2 |           85 |          284 |        369 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    630 us
  parse       bench.c       :    128 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    117 us
  link        bench_rcc     :    281 us
  link        bench_rcc     :  47674 us

RCC -O1:
  preprocess  bench.c       :    636 us
  parse       bench.c       :    134 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    122 us
  link        bench_o1      :    121 us
  link        bench_o1      :  48008 us

RCC -O2:
  preprocess  bench.c       :    590 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    119 us
  link        bench_o2      :    119 us
  link        bench_o2      :  47354 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 197347 us
  parse       sqlite3.c     :  51123 us
  typecheck   sqlite3.c     :  11639 us
  codegen     sqlite3.c     : 104342 us
  link        sqlite3.so    :  16536 us

RCC -O1:
  preprocess  sqlite3.c     : 220774 us
  parse       sqlite3.c     :  59362 us
  typecheck   sqlite3.c     :  11453 us
  opt         sqlite3.c     : 138583 us
  codegen     sqlite3.c     : 134393 us
  link        sqlite3.so    :  15939 us

RCC -O2:
  preprocess  sqlite3.c     : 225736 us
  parse       sqlite3.c     :  56734 us
  typecheck   sqlite3.c     :  14291 us
  opt         sqlite3.c     : 153661 us
  codegen     sqlite3.c     :  98692 us
  link        sqlite3.so    :  15362 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       764 ms |
| RCC -O1   |       732 ms |
| RCC -O2   |       768 ms |
| TCC       |        94 ms |
| GCC -O0   |       999 ms |
| GCC -O2   |      9734 ms |
| Clang -O0 |      1011 ms |
| Clang -O2 |      9520 ms |
