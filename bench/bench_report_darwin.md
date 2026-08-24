# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          133 |          697 |        830 |
| RCC -O1   |           86 |          769 |        855 |
| RCC -O2   |           78 |          798 |        876 |
| TCC       |           72 |          696 |        768 |
| GCC -O0   |           99 |          583 |        682 |
| GCC -O2   |          144 |          340 |        484 |
| Clang -O0 |           83 |          588 |        671 |
| Clang -O2 |          141 |          334 |        475 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1777 us
  parse       bench.c       :    424 us
  typecheck   bench.c       :     10 us
  codegen     bench.c       :    319 us
  link        bench_rcc     :    482 us
  link        bench_rcc     :  79730 us

RCC -O1:
  preprocess  bench.c       :    724 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    210 us
  link        bench_o1      :    413 us
  link        bench_o1      :  74575 us

RCC -O2:
  preprocess  bench.c       :   1605 us
  parse       bench.c       :    161 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     45 us
  codegen     bench.c       :    144 us
  link        bench_o2      :    312 us
  link        bench_o2      :  65711 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 312156 us
  parse       sqlite3.c     : 183783 us
  typecheck   sqlite3.c     :  41462 us
  codegen     sqlite3.c     : 201311 us
  link        sqlite3.so    :  34279 us

RCC -O1:
  preprocess  sqlite3.c     : 330225 us
  parse       sqlite3.c     :  86925 us
  typecheck   sqlite3.c     :  18221 us
  opt         sqlite3.c     : 243801 us
  codegen     sqlite3.c     : 128350 us
  link        sqlite3.so    :  29716 us

RCC -O2:
  preprocess  sqlite3.c     : 395239 us
  parse       sqlite3.c     :  84249 us
  typecheck   sqlite3.c     :  21224 us
  opt         sqlite3.c     : 218444 us
  codegen     sqlite3.c     : 236875 us
  link        sqlite3.so    :  27716 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1256 ms |
| RCC -O1   |      1128 ms |
| RCC -O2   |      1033 ms |
| TCC       |       129 ms |
| GCC -O0   |      1387 ms |
| GCC -O2   |     14504 ms |
| Clang -O0 |      1628 ms |
| Clang -O2 |     12948 ms |
