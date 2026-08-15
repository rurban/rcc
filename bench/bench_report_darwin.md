# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           62 |          682 |        744 |
| RCC -O1   |           64 |          676 |        740 |
| RCC -O2   |           62 |          688 |        750 |
| TCC       |           49 |          595 |        644 |
| GCC -O0   |           77 |          504 |        581 |
| GCC -O2   |          113 |          299 |        412 |
| Clang -O0 |           65 |          491 |        556 |
| Clang -O2 |          108 |          302 |        410 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1584 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    173 us
  link        bench_rcc     :    166 us
  link        bench_rcc     :  54226 us

RCC -O1:
  preprocess  bench.c       :    842 us
  parse       bench.c       :    218 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    186 us
  link        bench_o1      :    202 us
  link        bench_o1      :  59817 us

RCC -O2:
  preprocess  bench.c       :    805 us
  parse       bench.c       :    178 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    138 us
  link        bench_o2      :    138 us
  link        bench_o2      :  63173 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 282791 us
  parse       sqlite3.c     :  62725 us
  typecheck   sqlite3.c     :  14248 us
  codegen     sqlite3.c     : 112670 us
  link        sqlite3.so    :  17228 us

RCC -O1:
  preprocess  sqlite3.c     : 253320 us
  parse       sqlite3.c     :  52305 us
  typecheck   sqlite3.c     :  14287 us
  opt         sqlite3.c     : 151991 us
  codegen     sqlite3.c     : 108019 us
  link        sqlite3.so    :  17837 us

RCC -O2:
  preprocess  sqlite3.c     : 250840 us
  parse       sqlite3.c     :  56243 us
  typecheck   sqlite3.c     :  14930 us
  opt         sqlite3.c     : 147973 us
  codegen     sqlite3.c     : 109636 us
  link        sqlite3.so    :  18132 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       700 ms |
| RCC -O1   |       859 ms |
| RCC -O2   |       816 ms |
| TCC       |       109 ms |
| GCC -O0   |      1154 ms |
| GCC -O2   |     12165 ms |
| Clang -O0 |      1198 ms |
| Clang -O2 |     11310 ms |
