# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           97 |          857 |        954 |
| RCC -O1   |          106 |          778 |        884 |
| RCC -O2   |          143 |          878 |       1021 |
| TCC       |           84 |          713 |        797 |
| GCC -O0   |          108 |          659 |        767 |
| GCC -O2   |          257 |          396 |        653 |
| Clang -O0 |           98 |          728 |        826 |
| Clang -O2 |          193 |          384 |        577 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1826 us
  parse       bench.c       :    135 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    139 us
  link        bench_rcc     :    805 us
  link        bench_rcc     :  79125 us

RCC -O1:
  preprocess  bench.c       :    922 us
  parse       bench.c       :    230 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    145 us
  link        bench_o1      :    460 us
  link        bench_o1      :  76227 us

RCC -O2:
  preprocess  bench.c       :    875 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    129 us
  link        bench_o2      :    231 us
  link        bench_o2      :  72624 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 432862 us
  parse       sqlite3.c     : 221825 us
  typecheck   sqlite3.c     :  26388 us
  codegen     sqlite3.c     : 196987 us
  link        sqlite3.so    :  22416 us

RCC -O1:
  preprocess  sqlite3.c     : 360823 us
  parse       sqlite3.c     :  89194 us
  typecheck   sqlite3.c     :  32834 us
  opt         sqlite3.c     : 273649 us
  codegen     sqlite3.c     : 179611 us
  link        sqlite3.so    :  20027 us

RCC -O2:
  preprocess  sqlite3.c     : 377735 us
  parse       sqlite3.c     :  68627 us
  typecheck   sqlite3.c     :  24886 us
  opt         sqlite3.c     : 257456 us
  codegen     sqlite3.c     : 181953 us
  link        sqlite3.so    :  20493 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1127 ms |
| RCC -O1   |      1233 ms |
| RCC -O2   |      1442 ms |
| TCC       |       218 ms |
| GCC -O0   |      1892 ms |
| GCC -O2   |     18436 ms |
| Clang -O0 |      1879 ms |
| Clang -O2 |     19034 ms |
