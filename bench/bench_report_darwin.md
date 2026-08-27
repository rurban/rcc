# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          287 |          779 |       1066 |
| RCC -O1   |           96 |          735 |        831 |
| RCC -O2   |           78 |          838 |        916 |
| TCC       |          117 |          650 |        767 |
| GCC -O0   |          131 |          562 |        693 |
| GCC -O2   |          139 |          300 |        439 |
| Clang -O0 |           69 |          516 |        585 |
| Clang -O2 |          122 |          301 |        423 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1861 us
  parse       bench.c       :    353 us
  typecheck   bench.c       :     10 us
  codegen     bench.c       :    334 us
  link        bench_rcc     :    208 us
  link        bench_rcc     :  88839 us

RCC -O1:
  preprocess  bench.c       :    830 us
  parse       bench.c       :    150 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    160 us
  link        bench_o1      :    281 us
  link        bench_o1      :  80405 us

RCC -O2:
  preprocess  bench.c       :   1087 us
  parse       bench.c       :    135 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    351 us
  link        bench_o2      :    141 us
  link        bench_o2      :  90712 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 286614 us
  parse       sqlite3.c     :  76609 us
  typecheck   sqlite3.c     :  16794 us
  codegen     sqlite3.c     : 216910 us
  link        sqlite3.so    :  17350 us

RCC -O1:
  preprocess  sqlite3.c     : 309969 us
  parse       sqlite3.c     :  95058 us
  typecheck   sqlite3.c     :  23774 us
  opt         sqlite3.c     : 256616 us
  codegen     sqlite3.c     : 165396 us
  link        sqlite3.so    :  27136 us

RCC -O2:
  preprocess  sqlite3.c     : 326938 us
  parse       sqlite3.c     :  60244 us
  typecheck   sqlite3.c     :  18103 us
  opt         sqlite3.c     : 316120 us
  codegen     sqlite3.c     : 213223 us
  link        sqlite3.so    :  30139 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1155 ms |
| RCC -O1   |      1230 ms |
| RCC -O2   |       953 ms |
| TCC       |       108 ms |
| GCC -O0   |      1194 ms |
| GCC -O2   |     13666 ms |
| Clang -O0 |      2754 ms |
| Clang -O2 |     13827 ms |
