# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          102 |          741 |        843 |
| RCC -O1   |           81 |          723 |        804 |
| RCC -O2   |           89 |          714 |        803 |
| TCC       |           66 |          607 |        673 |
| GCC -O0   |          170 |          790 |        960 |
| GCC -O2   |          221 |          383 |        604 |
| Clang -O0 |          157 |          611 |        768 |
| Clang -O2 |          277 |          413 |        690 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1296 us
  parse       bench.c       :    163 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    155 us
  link        bench_rcc     :    215 us
  link        bench_rcc     :  70773 us

RCC -O1:
  preprocess  bench.c       :    655 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    124 us
  link        bench_o1      :    629 us
  link        bench_o1      :  59414 us

RCC -O2:
  preprocess  bench.c       :    564 us
  parse       bench.c       :    141 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    124 us
  link        bench_o2      :     82 us
  link        bench_o2      :  83454 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 412428 us
  parse       sqlite3.c     : 206997 us
  typecheck   sqlite3.c     :  30143 us
  codegen     sqlite3.c     : 193764 us
  link        sqlite3.so    :  22337 us

RCC -O1:
  preprocess  sqlite3.c     : 351862 us
  parse       sqlite3.c     :  92443 us
  typecheck   sqlite3.c     :  30075 us
  opt         sqlite3.c     :  22264 us
  codegen     sqlite3.c     : 153782 us
  link        sqlite3.so    :  19679 us

RCC -O2:
  preprocess  sqlite3.c     : 362300 us
  parse       sqlite3.c     :  70759 us
  typecheck   sqlite3.c     :  18525 us
  opt         sqlite3.c     : 199063 us
  codegen     sqlite3.c     : 132590 us
  link        sqlite3.so    :  20727 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1292 ms |
| RCC -O1   |       916 ms |
| RCC -O2   |      1323 ms |
| TCC       |       170 ms |
| GCC -O0   |      1579 ms |
| GCC -O2   |     15867 ms |
| Clang -O0 |      1591 ms |
| Clang -O2 |     14743 ms |
