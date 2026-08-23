# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           51 |          655 |        706 |
| RCC -O1   |           56 |          656 |        712 |
| RCC -O2   |           57 |          662 |        719 |
| TCC       |           44 |          545 |        589 |
| GCC -O0   |           70 |          479 |        549 |
| GCC -O2   |          126 |          266 |        392 |
| Clang -O0 |           65 |          451 |        516 |
| Clang -O2 |           93 |          268 |        361 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    798 us
  parse       bench.c       :    230 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    174 us
  link        bench_rcc     :     90 us
  link        bench_rcc     :  52347 us

RCC -O1:
  preprocess  bench.c       :    662 us
  parse       bench.c       :    172 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    151 us
  link        bench_o1      :    370 us
  link        bench_o1      :  49608 us

RCC -O2:
  preprocess  bench.c       :    603 us
  parse       bench.c       :    192 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    152 us
  link        bench_o2      :    374 us
  link        bench_o2      :  53037 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 201763 us
  parse       sqlite3.c     :  77375 us
  typecheck   sqlite3.c     :  14529 us
  codegen     sqlite3.c     :  92211 us
  link        sqlite3.so    :  14792 us

RCC -O1:
  preprocess  sqlite3.c     : 180370 us
  parse       sqlite3.c     :  44886 us
  typecheck   sqlite3.c     :  11776 us
  opt         sqlite3.c     : 122296 us
  codegen     sqlite3.c     : 105912 us
  link        sqlite3.so    :  15521 us

RCC -O2:
  preprocess  sqlite3.c     : 174837 us
  parse       sqlite3.c     :  43707 us
  typecheck   sqlite3.c     :  12105 us
  opt         sqlite3.c     : 124895 us
  codegen     sqlite3.c     : 106734 us
  link        sqlite3.so    :  15883 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1001 ms |
| RCC -O1   |       974 ms |
| RCC -O2   |       780 ms |
| TCC       |       110 ms |
| GCC -O0   |      1411 ms |
| GCC -O2   |     12491 ms |
| Clang -O0 |      1064 ms |
| Clang -O2 |      9547 ms |
