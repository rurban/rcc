# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          162 |          777 |        939 |
| RCC -O1   |           70 |          714 |        784 |
| RCC -O2   |           73 |          749 |        822 |
| TCC       |           59 |          677 |        736 |
| GCC -O0   |          105 |          575 |        680 |
| GCC -O2   |          170 |          303 |        473 |
| Clang -O0 |           69 |          558 |        627 |
| Clang -O2 |          117 |          313 |        430 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    812 us
  parse       bench.c       :    186 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    164 us
  link        bench_rcc     :     97 us
  link        bench_rcc     :  52441 us

RCC -O1:
  preprocess  bench.c       :    685 us
  parse       bench.c       :    134 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    110 us
  link        bench_o1      :    421 us
  link        bench_o1      :  56476 us

RCC -O2:
  preprocess  bench.c       :    590 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    112 us
  link        bench_o2      :    279 us
  link        bench_o2      :  49484 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 429569 us
  parse       sqlite3.c     : 204706 us
  typecheck   sqlite3.c     :  25092 us
  codegen     sqlite3.c     : 187416 us
  link        sqlite3.so    :  47203 us

RCC -O1:
  preprocess  sqlite3.c     : 373430 us
  parse       sqlite3.c     :  71929 us
  typecheck   sqlite3.c     :  26868 us
  opt         sqlite3.c     :  30205 us
  codegen     sqlite3.c     : 164263 us
  link        sqlite3.so    :  20883 us

RCC -O2:
  preprocess  sqlite3.c     : 352643 us
  parse       sqlite3.c     :  95932 us
  typecheck   sqlite3.c     :  16403 us
  opt         sqlite3.c     : 214354 us
  codegen     sqlite3.c     : 157863 us
  link        sqlite3.so    :  22609 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1258 ms |
| RCC -O1   |       937 ms |
| RCC -O2   |      1315 ms |
| TCC       |       724 ms |
| GCC -O0   |      1378 ms |
| GCC -O2   |     12521 ms |
| Clang -O0 |      1583 ms |
| Clang -O2 |     14920 ms |
