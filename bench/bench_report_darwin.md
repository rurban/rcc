# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           55 |          625 |        680 |
| RCC -O1   |           58 |          626 |        684 |
| RCC -O2   |           54 |          628 |        682 |
| TCC       |           46 |          557 |        603 |
| GCC -O0   |           77 |          468 |        545 |
| GCC -O2   |          103 |          284 |        387 |
| Clang -O0 |           57 |          469 |        526 |
| Clang -O2 |           86 |          289 |        375 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    651 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    113 us
  link        bench_rcc     :     57 us
  link        bench_rcc     :  61738 us

RCC -O1:
  preprocess  bench.c       :    605 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    130 us
  link        bench_o1      :    440 us
  link        bench_o1      :  72710 us

RCC -O2:
  preprocess  bench.c       :    637 us
  parse       bench.c       :    220 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    114 us
  link        bench_o2      :     73 us
  link        bench_o2      :  53876 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 247204 us
  parse       sqlite3.c     :  52688 us
  typecheck   sqlite3.c     :  14142 us
  codegen     sqlite3.c     : 111592 us
  link        sqlite3.so    :  15129 us

RCC -O1:
  preprocess  sqlite3.c     : 214618 us
  parse       sqlite3.c     :  43285 us
  typecheck   sqlite3.c     :  11773 us
  opt         sqlite3.c     :  19309 us
  codegen     sqlite3.c     :  96154 us
  link        sqlite3.so    :  15004 us

RCC -O2:
  preprocess  sqlite3.c     : 207354 us
  parse       sqlite3.c     :  45542 us
  typecheck   sqlite3.c     :  13056 us
  opt         sqlite3.c     : 135165 us
  codegen     sqlite3.c     :  94601 us
  link        sqlite3.so    :  15454 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       605 ms |
| RCC -O1   |       679 ms |
| RCC -O2   |       727 ms |
| TCC       |        93 ms |
| GCC -O0   |      1033 ms |
| GCC -O2   |     10246 ms |
| Clang -O0 |      1086 ms |
| Clang -O2 |     10182 ms |
