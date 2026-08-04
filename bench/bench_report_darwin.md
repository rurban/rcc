# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          151 |          786 |        937 |
| RCC -O1   |          105 |          689 |        794 |
| RCC -O2   |           86 |          628 |        714 |
| TCC       |           50 |          539 |        589 |
| GCC -O0   |          119 |          462 |        581 |
| GCC -O2   |          218 |          356 |        574 |
| Clang -O0 |           98 |          525 |        623 |
| Clang -O2 |          124 |          300 |        424 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    827 us
  parse       bench.c       :    163 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    188 us
  link        bench_rcc     :    168 us
  link        bench_rcc     :  74868 us

RCC -O1:
  preprocess  bench.c       :    805 us
  parse       bench.c       :    207 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    162 us
  link        bench_o1      :    294 us
  link        bench_o1      :  71502 us

RCC -O2:
  preprocess  bench.c       :    807 us
  parse       bench.c       :    202 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    173 us
  link        bench_o2      :    257 us
  link        bench_o2      :  72929 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 422732 us
  parse       sqlite3.c     :  85541 us
  typecheck   sqlite3.c     :  19793 us
  codegen     sqlite3.c     : 192269 us
  link        sqlite3.so    :  15978 us

RCC -O1:
  preprocess  sqlite3.c     : 332947 us
  parse       sqlite3.c     :  90089 us
  typecheck   sqlite3.c     :  14866 us
  opt         sqlite3.c     :  20153 us
  codegen     sqlite3.c     : 119909 us
  link        sqlite3.so    :  19486 us

RCC -O2:
  preprocess  sqlite3.c     : 292445 us
  parse       sqlite3.c     :  55113 us
  typecheck   sqlite3.c     :  12677 us
  opt         sqlite3.c     : 268360 us
  codegen     sqlite3.c     : 132452 us
  link        sqlite3.so    :  16206 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1048 ms |
| RCC -O1   |       743 ms |
| RCC -O2   |      1183 ms |
| TCC       |       542 ms |
| GCC -O0   |      2226 ms |
| GCC -O2   |     11781 ms |
| Clang -O0 |      1406 ms |
| Clang -O2 |     11661 ms |
