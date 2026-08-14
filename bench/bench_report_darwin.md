# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           78 |          736 |        814 |
| RCC -O1   |           74 |          728 |        802 |
| RCC -O2   |           88 |          707 |        795 |
| TCC       |           54 |          649 |        703 |
| GCC -O0   |           89 |          548 |        637 |
| GCC -O2   |          112 |          319 |        431 |
| Clang -O0 |           79 |          504 |        583 |
| Clang -O2 |          146 |          352 |        498 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    830 us
  parse       bench.c       :    160 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    160 us
  link        bench_rcc     :     73 us
  link        bench_rcc     :  58937 us

RCC -O1:
  preprocess  bench.c       :    681 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    146 us
  link        bench_o1      :    132 us
  link        bench_o1      :  60773 us

RCC -O2:
  preprocess  bench.c       :    633 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    147 us
  link        bench_o2      :    126 us
  link        bench_o2      :  59635 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 342024 us
  parse       sqlite3.c     : 115650 us
  typecheck   sqlite3.c     :  21848 us
  codegen     sqlite3.c     : 192551 us
  link        sqlite3.so    :  17883 us

RCC -O1:
  preprocess  sqlite3.c     : 354590 us
  parse       sqlite3.c     :  78321 us
  typecheck   sqlite3.c     :  26899 us
  opt         sqlite3.c     : 223517 us
  codegen     sqlite3.c     : 120970 us
  link        sqlite3.so    :  19175 us

RCC -O2:
  preprocess  sqlite3.c     : 320554 us
  parse       sqlite3.c     :  63946 us
  typecheck   sqlite3.c     :  18220 us
  opt         sqlite3.c     : 192907 us
  codegen     sqlite3.c     : 216436 us
  link        sqlite3.so    :  20388 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1202 ms |
| RCC -O1   |      1383 ms |
| RCC -O2   |      1603 ms |
| TCC       |       282 ms |
| GCC -O0   |      2806 ms |
| GCC -O2   |     16842 ms |
| Clang -O0 |      1430 ms |
| Clang -O2 |     14593 ms |
