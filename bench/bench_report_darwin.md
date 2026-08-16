# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          109 |          674 |        783 |
| RCC -O1   |           83 |          707 |        790 |
| RCC -O2   |          123 |          657 |        780 |
| TCC       |           60 |          565 |        625 |
| GCC -O0   |           98 |          474 |        572 |
| GCC -O2   |          123 |          288 |        411 |
| Clang -O0 |           72 |          475 |        547 |
| Clang -O2 |          107 |          289 |        396 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1569 us
  parse       bench.c       :    336 us
  typecheck   bench.c       :     11 us
  codegen     bench.c       :    359 us
  link        bench_rcc     :    485 us
  link        bench_rcc     :  74167 us

RCC -O1:
  preprocess  bench.c       :   1342 us
  parse       bench.c       :    175 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    124 us
  link        bench_o1      :    153 us
  link        bench_o1      :  65241 us

RCC -O2:
  preprocess  bench.c       :    696 us
  parse       bench.c       :    135 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    117 us
  link        bench_o2      :    159 us
  link        bench_o2      :  59371 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 274273 us
  parse       sqlite3.c     :  61169 us
  typecheck   sqlite3.c     :  22417 us
  codegen     sqlite3.c     : 109415 us
  link        sqlite3.so    :  16353 us

RCC -O1:
  preprocess  sqlite3.c     : 263782 us
  parse       sqlite3.c     :  50518 us
  typecheck   sqlite3.c     :  17109 us
  opt         sqlite3.c     : 159516 us
  codegen     sqlite3.c     : 120716 us
  link        sqlite3.so    :  15352 us

RCC -O2:
  preprocess  sqlite3.c     : 252158 us
  parse       sqlite3.c     :  54767 us
  typecheck   sqlite3.c     :  27870 us
  opt         sqlite3.c     : 168553 us
  codegen     sqlite3.c     : 113330 us
  link        sqlite3.so    :  21767 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       896 ms |
| RCC -O1   |       833 ms |
| RCC -O2   |       774 ms |
| TCC       |       115 ms |
| GCC -O0   |      1076 ms |
| GCC -O2   |     11957 ms |
| Clang -O0 |      1473 ms |
| Clang -O2 |     11186 ms |
