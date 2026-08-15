# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           55 |          636 |        691 |
| RCC -O1   |           54 |          635 |        689 |
| RCC -O2   |           51 |          639 |        690 |
| TCC       |           44 |          554 |        598 |
| GCC -O0   |           63 |          471 |        534 |
| GCC -O2   |          108 |          283 |        391 |
| Clang -O0 |           58 |          479 |        537 |
| Clang -O2 |          141 |          301 |        442 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    623 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    120 us
  link        bench_rcc     :     98 us
  link        bench_rcc     :  47007 us

RCC -O1:
  preprocess  bench.c       :    587 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    113 us
  link        bench_o1      :    108 us
  link        bench_o1      :  47703 us

RCC -O2:
  preprocess  bench.c       :    597 us
  parse       bench.c       :    119 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    120 us
  link        bench_o2      :    124 us
  link        bench_o2      :  46935 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 209177 us
  parse       sqlite3.c     :  48741 us
  typecheck   sqlite3.c     :  12923 us
  codegen     sqlite3.c     :  94744 us
  link        sqlite3.so    :  15140 us

RCC -O1:
  preprocess  sqlite3.c     : 208398 us
  parse       sqlite3.c     :  47041 us
  typecheck   sqlite3.c     :  13100 us
  opt         sqlite3.c     : 133662 us
  codegen     sqlite3.c     :  96113 us
  link        sqlite3.so    :  14954 us

RCC -O2:
  preprocess  sqlite3.c     : 208550 us
  parse       sqlite3.c     :  46615 us
  typecheck   sqlite3.c     :  13228 us
  opt         sqlite3.c     : 133436 us
  codegen     sqlite3.c     :  96593 us
  link        sqlite3.so    :  15767 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       703 ms |
| RCC -O1   |       880 ms |
| RCC -O2   |       811 ms |
| TCC       |       129 ms |
| GCC -O0   |      1208 ms |
| GCC -O2   |     12844 ms |
| Clang -O0 |      1234 ms |
| Clang -O2 |     13674 ms |
