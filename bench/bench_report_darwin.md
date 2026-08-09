# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           84 |          643 |        727 |
| RCC -O1   |           59 |          636 |        695 |
| RCC -O2   |           95 |          734 |        829 |
| TCC       |           81 |          589 |        670 |
| GCC -O0   |          136 |          469 |        605 |
| GCC -O2   |          116 |          289 |        405 |
| Clang -O0 |           69 |          476 |        545 |
| Clang -O2 |          113 |          328 |        441 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    782 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    151 us
  link        bench_rcc     :    230 us
  link        bench_rcc     :  49467 us

RCC -O1:
  preprocess  bench.c       :    598 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    130 us
  link        bench_o1      :    502 us
  link        bench_o1      :  48933 us

RCC -O2:
  preprocess  bench.c       :    640 us
  parse       bench.c       :    166 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    111 us
  link        bench_o2      :    483 us
  link        bench_o2      :  49380 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 315896 us
  parse       sqlite3.c     : 160334 us
  typecheck   sqlite3.c     :  21692 us
  codegen     sqlite3.c     :  98835 us
  link        sqlite3.so    :  16560 us

RCC -O1:
  preprocess  sqlite3.c     : 271251 us
  parse       sqlite3.c     :  46088 us
  typecheck   sqlite3.c     :  12421 us
  opt         sqlite3.c     :  18834 us
  codegen     sqlite3.c     : 170250 us
  link        sqlite3.so    :  17120 us

RCC -O2:
  preprocess  sqlite3.c     : 254385 us
  parse       sqlite3.c     :  47238 us
  typecheck   sqlite3.c     :  13596 us
  opt         sqlite3.c     : 171610 us
  codegen     sqlite3.c     : 118050 us
  link        sqlite3.so    :  19685 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       846 ms |
| RCC -O1   |       871 ms |
| RCC -O2   |      1298 ms |
| TCC       |       127 ms |
| GCC -O0   |      1321 ms |
| GCC -O2   |     12153 ms |
| Clang -O0 |      1213 ms |
| Clang -O2 |     10247 ms |
