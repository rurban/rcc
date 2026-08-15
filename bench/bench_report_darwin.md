# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           63 |          655 |        718 |
| RCC -O1   |           68 |          623 |        691 |
| RCC -O2   |           55 |          622 |        677 |
| TCC       |           58 |          540 |        598 |
| GCC -O0   |          131 |          486 |        617 |
| GCC -O2   |          119 |          295 |        414 |
| Clang -O0 |           65 |          455 |        520 |
| Clang -O2 |           87 |          276 |        363 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    694 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    138 us
  link        bench_rcc     :     83 us
  link        bench_rcc     :  53541 us

RCC -O1:
  preprocess  bench.c       :    584 us
  parse       bench.c       :    184 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    128 us
  link        bench_o1      :    129 us
  link        bench_o1      :  48545 us

RCC -O2:
  preprocess  bench.c       :    780 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    127 us
  link        bench_o2      :    110 us
  link        bench_o2      :  47664 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 222761 us
  parse       sqlite3.c     :  55403 us
  typecheck   sqlite3.c     :  13809 us
  codegen     sqlite3.c     : 140719 us
  link        sqlite3.so    :  17566 us

RCC -O1:
  preprocess  sqlite3.c     : 231292 us
  parse       sqlite3.c     :  50440 us
  typecheck   sqlite3.c     :  13239 us
  opt         sqlite3.c     : 138322 us
  codegen     sqlite3.c     : 106901 us
  link        sqlite3.so    :  15233 us

RCC -O2:
  preprocess  sqlite3.c     : 203088 us
  parse       sqlite3.c     :  49579 us
  typecheck   sqlite3.c     :  13505 us
  opt         sqlite3.c     : 166918 us
  codegen     sqlite3.c     : 121588 us
  link        sqlite3.so    :  16643 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       707 ms |
| RCC -O1   |       732 ms |
| RCC -O2   |       713 ms |
| TCC       |       100 ms |
| GCC -O0   |      1005 ms |
| GCC -O2   |     10524 ms |
| Clang -O0 |      1205 ms |
| Clang -O2 |      9749 ms |
