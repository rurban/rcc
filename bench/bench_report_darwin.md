# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           71 |          649 |        720 |
| RCC -O1   |           55 |          689 |        744 |
| RCC -O2   |           75 |          697 |        772 |
| TCC       |          122 |          687 |        809 |
| GCC -O0   |           88 |          485 |        573 |
| GCC -O2   |          174 |          298 |        472 |
| Clang -O0 |           75 |          620 |        695 |
| Clang -O2 |          122 |          366 |        488 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    686 us
  parse       bench.c       :    180 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    200 us
  link        bench_rcc     :     82 us
  link        bench_rcc     :  52254 us

RCC -O1:
  preprocess  bench.c       :    740 us
  parse       bench.c       :    157 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    139 us
  link        bench_o1      :    587 us
  link        bench_o1      :  70579 us

RCC -O2:
  preprocess  bench.c       :    733 us
  parse       bench.c       :    150 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     82 us
  codegen     bench.c       :    183 us
  link        bench_o2      :     89 us
  link        bench_o2      :  56369 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 376254 us
  parse       sqlite3.c     :  54565 us
  typecheck   sqlite3.c     :  21078 us
  codegen     sqlite3.c     : 104920 us
  link        sqlite3.so    :  17562 us

RCC -O1:
  preprocess  sqlite3.c     : 239818 us
  parse       sqlite3.c     :  53936 us
  typecheck   sqlite3.c     :  15413 us
  opt         sqlite3.c     : 149451 us
  codegen     sqlite3.c     : 114554 us
  link        sqlite3.so    :  20942 us

RCC -O2:
  preprocess  sqlite3.c     : 268967 us
  parse       sqlite3.c     :  62016 us
  typecheck   sqlite3.c     :  19959 us
  opt         sqlite3.c     : 155129 us
  codegen     sqlite3.c     : 140304 us
  link        sqlite3.so    :  16743 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1139 ms |
| RCC -O1   |       894 ms |
| RCC -O2   |      1001 ms |
| TCC       |       133 ms |
| GCC -O0   |      1256 ms |
| GCC -O2   |     14877 ms |
| Clang -O0 |      1644 ms |
| Clang -O2 |     15545 ms |
