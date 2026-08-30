# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          135 |          749 |        884 |
| RCC -O1   |           69 |          758 |        827 |
| RCC -O2   |           60 |          739 |        799 |
| TCC       |           54 |          714 |        768 |
| GCC -O0   |          101 |          566 |        667 |
| GCC -O2   |          155 |          374 |        529 |
| Clang -O0 |          104 |          616 |        720 |
| Clang -O2 |          154 |          347 |        501 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1052 us
  parse       bench.c       :    207 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    212 us
  link        bench_rcc     :    344 us
  link        bench_rcc     :  69239 us

RCC -O1:
  preprocess  bench.c       :   1040 us
  parse       bench.c       :    204 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    139 us
  link        bench_o1      :    387 us
  link        bench_o1      :  58166 us

RCC -O2:
  preprocess  bench.c       :    664 us
  parse       bench.c       :    156 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    134 us
  link        bench_o2      :    333 us
  link        bench_o2      :  66737 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 273046 us
  parse       sqlite3.c     : 155679 us
  typecheck   sqlite3.c     :  26845 us
  codegen     sqlite3.c     : 137690 us
  link        sqlite3.so    :  15894 us

RCC -O1:
  preprocess  sqlite3.c     : 319692 us
  parse       sqlite3.c     :  77641 us
  typecheck   sqlite3.c     :  18821 us
  opt         sqlite3.c     : 206085 us
  codegen     sqlite3.c     : 159576 us
  link        sqlite3.so    :  17250 us

RCC -O2:
  preprocess  sqlite3.c     : 254893 us
  parse       sqlite3.c     :  74304 us
  typecheck   sqlite3.c     :  20365 us
  opt         sqlite3.c     : 210703 us
  codegen     sqlite3.c     : 178163 us
  link        sqlite3.so    :  31452 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1791 ms |
| RCC -O1   |      1357 ms |
| RCC -O2   |      1312 ms |
| TCC       |       190 ms |
| GCC -O0   |      1626 ms |
| GCC -O2   |     15792 ms |
| Clang -O0 |      1540 ms |
| Clang -O2 |     16283 ms |
