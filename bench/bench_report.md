# Linux RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           17 |          706 |        723 |
| RCC -O1   |           15 |          692 |        707 |
| RCC -O2   |           16 |          685 |        701 |
| TCC       |           13 |          565 |        578 |
| SLIMCC    |           52 |          643 |        695 |
| XCC       |           24 |          364 |        388 |
| KEFIR     |          212 |          676 |        888 |
| KEFIR -O1 |          191 |          497 |        688 |
| SCC       |           57 |          584 |        641 |
| ANTCC     |           30 |          475 |        505 |
| CCC       |           34 |          559 |        593 |
| GCC -O0   |           75 |          568 |        643 |
| GCC -O2   |          165 |          206 |        371 |
| Clang -O0 |          233 |          634 |        867 |
| Clang -O2 |          131 |          233 |        364 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  11235 us
  parse       bench.c       :    859 us
  typecheck   bench.c       :     11 us
  codegen     bench.c       :    482 us
  link        bench_rcc     :    692 us

RCC -O1:
  preprocess  bench.c       :  14779 us
  parse       bench.c       :    832 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     36 us
  codegen     bench.c       :    450 us
  link        bench_o1      :    677 us

RCC -O2:
  preprocess  bench.c       :  10648 us
  parse       bench.c       :   1067 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     47 us
  codegen     bench.c       :    596 us
  link        bench_o2      :    762 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 276160 us
  parse       sqlite3.c     : 169772 us
  typecheck   sqlite3.c     :  14767 us
  codegen     sqlite3.c     : 214934 us
  link        sqlite3.so    :   7580 us

RCC -O1:
  preprocess  sqlite3.c     : 294023 us
  parse       sqlite3.c     : 179485 us
  typecheck   sqlite3.c     :  16883 us
  opt         sqlite3.c     : 191367 us
  codegen     sqlite3.c     : 218080 us
  link        sqlite3.so    :   8325 us

RCC -O2:
  preprocess  sqlite3.c     : 295083 us
  parse       sqlite3.c     : 171908 us
  typecheck   sqlite3.c     :  15289 us
  opt         sqlite3.c     : 196413 us
  codegen     sqlite3.c     : 215756 us
  link        sqlite3.so    :   8470 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1060 ms |
| RCC -O1   |      1217 ms |
| RCC -O2   |      1239 ms |
| TCC       |       115 ms |
| SLIMCC    |      1288 ms |
| KEFIR     |     23466 ms |
| KEFIR -O1 |     26294 ms |
| ANTCC     |       447 ms |
| CCC       |     17644 ms |
| GCC -O0   |      5172 ms |
| GCC -O2   |     32176 ms |
| Clang -O0 |      1473 ms |
| Clang -O2 |     18612 ms |
