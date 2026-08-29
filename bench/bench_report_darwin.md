# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          637 |        690 |
| RCC -O1   |           56 |          639 |        695 |
| RCC -O2   |           56 |          643 |        699 |
| TCC       |           51 |          554 |        605 |
| GCC -O0   |           71 |          467 |        538 |
| GCC -O2   |          120 |          284 |        404 |
| Clang -O0 |           60 |          485 |        545 |
| Clang -O2 |           95 |          284 |        379 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    762 us
  parse       bench.c       :    183 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    158 us
  link        bench_rcc     :     82 us
  link        bench_rcc     :  52967 us

RCC -O1:
  preprocess  bench.c       :    628 us
  parse       bench.c       :    160 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    141 us
  link        bench_o1      :    108 us
  link        bench_o1      :  50642 us

RCC -O2:
  preprocess  bench.c       :    690 us
  parse       bench.c       :    173 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    160 us
  link        bench_o2      :    112 us
  link        bench_o2      :  49368 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 249989 us
  parse       sqlite3.c     :  55481 us
  typecheck   sqlite3.c     :  11016 us
  codegen     sqlite3.c     :  98513 us
  link        sqlite3.so    :  17533 us

RCC -O1:
  preprocess  sqlite3.c     : 198789 us
  parse       sqlite3.c     :  51402 us
  typecheck   sqlite3.c     :  11064 us
  opt         sqlite3.c     : 138201 us
  codegen     sqlite3.c     :  97140 us
  link        sqlite3.so    :  15720 us

RCC -O2:
  preprocess  sqlite3.c     : 193945 us
  parse       sqlite3.c     :  51223 us
  typecheck   sqlite3.c     :  11371 us
  opt         sqlite3.c     : 141439 us
  codegen     sqlite3.c     :  96290 us
  link        sqlite3.so    :  15526 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       642 ms |
| RCC -O1   |       831 ms |
| RCC -O2   |       888 ms |
| TCC       |       122 ms |
| GCC -O0   |      1269 ms |
| GCC -O2   |     11532 ms |
| Clang -O0 |      1520 ms |
| Clang -O2 |     15966 ms |
