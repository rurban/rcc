# Linux RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           27 |          692 |        719 |
| RCC -O1   |           28 |          699 |        727 |
| RCC -O2   |           27 |          698 |        725 |
| TCC       |            9 |          490 |        499 |
| SLIMCC    |           36 |          503 |        539 |
| XCC       |           35 |          364 |        399 |
| KEFIR     |          235 |          577 |        812 |
| KEFIR -O1 |          202 |          312 |        514 |
| SCC       |          131 |          540 |        671 |
| LACC      |           40 |          759 |        799 |
| ANTCC     |           31 |          418 |        449 |
| CAKE      |          107 |          482 |        589 |
| CCC       |           36 |          540 |        576 |
| GCC -O0   |           59 |          483 |        542 |
| GCC -O2   |          159 |          179 |        338 |
| Clang -O0 |          300 |          468 |        768 |
| Clang -O2 |          260 |          181 |        441 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   9114 us
  parse       bench.c       :    589 us
  typecheck   bench.c       :     23 us
  codegen     bench.c       :    294 us
  link        bench_rcc     :  10597 us

RCC -O1:
  preprocess  bench.c       :   9340 us
  parse       bench.c       :    554 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    369 us
  link        bench_o1      :  10660 us

RCC -O2:
  preprocess  bench.c       :   9446 us
  parse       bench.c       :    625 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    298 us
  link        bench_o2      :  11461 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 253724 us
  parse       sqlite3.c     : 147387 us
  typecheck   sqlite3.c     :  10671 us
  codegen     sqlite3.c     : 202915 us
  link        sqlite3.so    :   9491 us

RCC -O1:
  preprocess  sqlite3.c     : 245642 us
  parse       sqlite3.c     : 145172 us
  typecheck   sqlite3.c     :  10765 us
  opt         sqlite3.c     : 218829 us
  codegen     sqlite3.c     : 196738 us
  link        sqlite3.so    :  10051 us

RCC -O2:
  preprocess  sqlite3.c     : 243116 us
  parse       sqlite3.c     : 144159 us
  typecheck   sqlite3.c     :  10957 us
  opt         sqlite3.c     : 229643 us
  codegen     sqlite3.c     : 197985 us
  link        sqlite3.so    :   9587 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       904 ms |
| RCC -O1   |      1140 ms |
| RCC -O2   |      1139 ms |
| TCC       |       112 ms |
| SLIMCC    |       666 ms |
| KEFIR     |     18386 ms |
| KEFIR -O1 |     33086 ms |
| ANTCC     |       398 ms |
| CCC       |     12648 ms |
| GCC -O0   |      4049 ms |
| GCC -O2   |     25645 ms |
| Clang -O0 |      1854 ms |
| Clang -O2 |     20046 ms |
