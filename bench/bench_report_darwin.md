# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          153 |          741 |        894 |
| RCC -O1   |          114 |          797 |        911 |
| RCC -O2   |          116 |          779 |        895 |
| TCC       |          108 |          766 |        874 |
| GCC -O0   |          197 |          612 |        809 |
| GCC -O2   |          200 |          315 |        515 |
| Clang -O0 |           97 |          519 |        616 |
| Clang -O2 |          196 |          325 |        521 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2302 us
  parse       bench.c       :   1434 us
  typecheck   bench.c       :      9 us
  codegen     bench.c       :   1134 us
  link        bench_rcc     :    923 us
  link        bench_rcc     :  72137 us

RCC -O1:
  preprocess  bench.c       :    842 us
  parse       bench.c       :    166 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    160 us
  link        bench_o1      :    600 us
  link        bench_o1      :  66101 us

RCC -O2:
  preprocess  bench.c       :   1861 us
  parse       bench.c       :    442 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    272 us
  link        bench_o2      :    511 us
  link        bench_o2      :  75727 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 412549 us
  parse       sqlite3.c     : 220551 us
  typecheck   sqlite3.c     :  33407 us
  codegen     sqlite3.c     : 248950 us
  link        sqlite3.so    :  20596 us

RCC -O1:
  preprocess  sqlite3.c     : 370880 us
  parse       sqlite3.c     :  80524 us
  typecheck   sqlite3.c     :  24117 us
  opt         sqlite3.c     : 210607 us
  codegen     sqlite3.c     : 191167 us
  link        sqlite3.so    :  23126 us

RCC -O2:
  preprocess  sqlite3.c     : 344371 us
  parse       sqlite3.c     : 104629 us
  typecheck   sqlite3.c     :  20798 us
  opt         sqlite3.c     : 210152 us
  codegen     sqlite3.c     : 178045 us
  link        sqlite3.so    :  29783 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1210 ms |
| RCC -O1   |       883 ms |
| RCC -O2   |       924 ms |
| TCC       |       134 ms |
| GCC -O0   |      1347 ms |
| GCC -O2   |     15592 ms |
| Clang -O0 |      2109 ms |
| Clang -O2 |     15502 ms |
