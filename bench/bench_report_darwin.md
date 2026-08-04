# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           73 |          646 |        719 |
| RCC -O1   |           57 |          643 |        700 |
| RCC -O2   |           60 |          630 |        690 |
| TCC       |           55 |          562 |        617 |
| GCC -O0   |           73 |          472 |        545 |
| GCC -O2   |          156 |          286 |        442 |
| Clang -O0 |           60 |          473 |        533 |
| Clang -O2 |           95 |          283 |        378 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    930 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    133 us
  link        bench_rcc     :    660 us
  link        bench_rcc     :  68401 us

RCC -O1:
  preprocess  bench.c       :   1354 us
  parse       bench.c       :    234 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    160 us
  link        bench_o1      :     60 us
  link        bench_o1      :  82879 us

RCC -O2:
  preprocess  bench.c       :    686 us
  parse       bench.c       :    175 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    142 us
  link        bench_o2      :    226 us
  link        bench_o2      :  55313 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 260198 us
  parse       sqlite3.c     :  47387 us
  typecheck   sqlite3.c     :  13795 us
  codegen     sqlite3.c     : 102052 us
  link        sqlite3.so    :  15975 us

RCC -O1:
  preprocess  sqlite3.c     : 224721 us
  parse       sqlite3.c     :  47309 us
  typecheck   sqlite3.c     :  12614 us
  opt         sqlite3.c     :  19587 us
  codegen     sqlite3.c     : 106153 us
  link        sqlite3.so    :  17854 us

RCC -O2:
  preprocess  sqlite3.c     : 272077 us
  parse       sqlite3.c     :  54701 us
  typecheck   sqlite3.c     :  14912 us
  opt         sqlite3.c     : 183089 us
  codegen     sqlite3.c     : 126180 us
  link        sqlite3.so    :  19094 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       625 ms |
| RCC -O1   |       655 ms |
| RCC -O2   |       917 ms |
| TCC       |       163 ms |
| GCC -O0   |      1534 ms |
| GCC -O2   |     13378 ms |
| Clang -O0 |      1283 ms |
| Clang -O2 |     10090 ms |
