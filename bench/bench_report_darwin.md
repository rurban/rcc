# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           64 |          629 |        693 |
| RCC -O1   |           49 |          615 |        664 |
| RCC -O2   |           66 |          652 |        718 |
| TCC       |           63 |          564 |        627 |
| GCC -O0   |           60 |          470 |        530 |
| GCC -O2   |           95 |          268 |        363 |
| Clang -O0 |           53 |          451 |        504 |
| Clang -O2 |          291 |          276 |        567 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    612 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    139 us
  link        bench_rcc     :    275 us
  link        bench_rcc     :  46944 us

RCC -O1:
  preprocess  bench.c       :    562 us
  parse       bench.c       :    178 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    117 us
  link        bench_o1      :    149 us
  link        bench_o1      :  45390 us

RCC -O2:
  preprocess  bench.c       :    546 us
  parse       bench.c       :    120 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    114 us
  link        bench_o2      :    154 us
  link        bench_o2      :  42159 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 210024 us
  parse       sqlite3.c     : 135829 us
  typecheck   sqlite3.c     :  24544 us
  codegen     sqlite3.c     :  92527 us
  link        sqlite3.so    :  14008 us

RCC -O1:
  preprocess  sqlite3.c     : 219231 us
  parse       sqlite3.c     :  45818 us
  typecheck   sqlite3.c     :  11759 us
  opt         sqlite3.c     : 118300 us
  codegen     sqlite3.c     :  87540 us
  link        sqlite3.so    :  14195 us

RCC -O2:
  preprocess  sqlite3.c     : 254093 us
  parse       sqlite3.c     :  53211 us
  typecheck   sqlite3.c     :  11769 us
  opt         sqlite3.c     : 123073 us
  codegen     sqlite3.c     :  87580 us
  link        sqlite3.so    :  14128 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       866 ms |
| RCC -O1   |       916 ms |
| RCC -O2   |       953 ms |
| TCC       |       127 ms |
| GCC -O0   |      1339 ms |
| GCC -O2   |     12319 ms |
| Clang -O0 |      1141 ms |
| Clang -O2 |     11706 ms |
