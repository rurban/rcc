# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           93 |          749 |        842 |
| RCC -O1   |           98 |          703 |        801 |
| RCC -O2   |           59 |          617 |        676 |
| TCC       |           56 |          582 |        638 |
| GCC -O0   |          154 |          508 |        662 |
| GCC -O2   |          128 |          328 |        456 |
| Clang -O0 |           75 |          457 |        532 |
| Clang -O2 |          112 |          279 |        391 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    888 us
  parse       bench.c       :    172 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    160 us
  link        bench_rcc     :    208 us
  link        bench_rcc     :  92234 us

RCC -O1:
  preprocess  bench.c       :   1153 us
  parse       bench.c       :    189 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    177 us
  link        bench_o1      :    205 us
  link        bench_o1      :  94522 us

RCC -O2:
  preprocess  bench.c       :   1351 us
  parse       bench.c       :    448 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     47 us
  codegen     bench.c       :    396 us
  link        bench_o2      :    145 us
  link        bench_o2      : 113317 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 369261 us
  parse       sqlite3.c     :  77384 us
  typecheck   sqlite3.c     :  23928 us
  codegen     sqlite3.c     : 182003 us
  link        sqlite3.so    :  21608 us

RCC -O1:
  preprocess  sqlite3.c     : 334576 us
  parse       sqlite3.c     :  80026 us
  typecheck   sqlite3.c     :  17396 us
  opt         sqlite3.c     : 182483 us
  codegen     sqlite3.c     : 144738 us
  link        sqlite3.so    :  21795 us

RCC -O2:
  preprocess  sqlite3.c     : 342901 us
  parse       sqlite3.c     :  79371 us
  typecheck   sqlite3.c     :  15145 us
  opt         sqlite3.c     : 196273 us
  codegen     sqlite3.c     : 161199 us
  link        sqlite3.so    :  17512 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       883 ms |
| RCC -O1   |       893 ms |
| RCC -O2   |       987 ms |
| TCC       |       120 ms |
| GCC -O0   |      1104 ms |
| GCC -O2   |     12918 ms |
| Clang -O0 |      1272 ms |
| Clang -O2 |     12070 ms |
