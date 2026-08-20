# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           50 |          603 |        653 |
| RCC -O1   |           52 |          592 |        644 |
| RCC -O2   |           49 |          602 |        651 |
| TCC       |           40 |          514 |        554 |
| GCC -O0   |           58 |          434 |        492 |
| GCC -O2   |           97 |          263 |        360 |
| Clang -O0 |           53 |          434 |        487 |
| Clang -O2 |           81 |          263 |        344 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    696 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    122 us
  link        bench_rcc     :     49 us
  link        bench_rcc     :  46006 us

RCC -O1:
  preprocess  bench.c       :    602 us
  parse       bench.c       :    135 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    128 us
  link        bench_o1      :    122 us
  link        bench_o1      :  45211 us

RCC -O2:
  preprocess  bench.c       :    590 us
  parse       bench.c       :    147 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    137 us
  link        bench_o2      :    161 us
  link        bench_o2      :  46360 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 210799 us
  parse       sqlite3.c     :  50114 us
  typecheck   sqlite3.c     :  12437 us
  codegen     sqlite3.c     :  87107 us
  link        sqlite3.so    :  16133 us

RCC -O1:
  preprocess  sqlite3.c     : 220769 us
  parse       sqlite3.c     :  53464 us
  typecheck   sqlite3.c     :  12982 us
  opt         sqlite3.c     : 126501 us
  codegen     sqlite3.c     :  84930 us
  link        sqlite3.so    :  13575 us

RCC -O2:
  preprocess  sqlite3.c     : 178271 us
  parse       sqlite3.c     :  44857 us
  typecheck   sqlite3.c     :  11662 us
  opt         sqlite3.c     : 140713 us
  codegen     sqlite3.c     :  85810 us
  link        sqlite3.so    :  14524 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       566 ms |
| RCC -O1   |       631 ms |
| RCC -O2   |       637 ms |
| TCC       |        85 ms |
| GCC -O0   |       914 ms |
| GCC -O2   |      9029 ms |
| Clang -O0 |       946 ms |
| Clang -O2 |      8830 ms |
