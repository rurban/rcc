# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           70 |          660 |        730 |
| RCC -O1   |           62 |          645 |        707 |
| RCC -O2   |           72 |          778 |        850 |
| TCC       |           58 |          621 |        679 |
| GCC -O0   |          104 |          541 |        645 |
| GCC -O2   |          195 |          287 |        482 |
| Clang -O0 |           56 |          468 |        524 |
| Clang -O2 |          104 |          285 |        389 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1289 us
  parse       bench.c       :    309 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    293 us
  link        bench_rcc     :    188 us
  link        bench_rcc     :  61396 us

RCC -O1:
  preprocess  bench.c       :    798 us
  parse       bench.c       :    181 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    180 us
  link        bench_o1      :     94 us
  link        bench_o1      :  62074 us

RCC -O2:
  preprocess  bench.c       :    728 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    171 us
  link        bench_o2      :    117 us
  link        bench_o2      :  56386 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 288254 us
  parse       sqlite3.c     :  79228 us
  typecheck   sqlite3.c     :  17385 us
  codegen     sqlite3.c     : 125396 us
  link        sqlite3.so    :  17955 us

RCC -O1:
  preprocess  sqlite3.c     : 283551 us
  parse       sqlite3.c     :  63744 us
  typecheck   sqlite3.c     :  16104 us
  opt         sqlite3.c     : 183341 us
  codegen     sqlite3.c     : 132386 us
  link        sqlite3.so    :  18678 us

RCC -O2:
  preprocess  sqlite3.c     : 320737 us
  parse       sqlite3.c     :  57987 us
  typecheck   sqlite3.c     :  13419 us
  opt         sqlite3.c     : 164217 us
  codegen     sqlite3.c     : 108176 us
  link        sqlite3.so    :  16039 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       696 ms |
| RCC -O1   |       740 ms |
| RCC -O2   |       712 ms |
| TCC       |       105 ms |
| GCC -O0   |      1052 ms |
| GCC -O2   |     10248 ms |
| Clang -O0 |      1071 ms |
| Clang -O2 |     10228 ms |
