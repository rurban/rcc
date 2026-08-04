# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          166 |          935 |       1101 |
| RCC -O1   |          168 |          941 |       1109 |
| RCC -O2   |          126 |         1016 |       1142 |
| TCC       |           85 |          782 |        867 |
| GCC -O0   |          170 |          697 |        867 |
| GCC -O2   |          191 |          362 |        553 |
| Clang -O0 |          188 |          659 |        847 |
| Clang -O2 |          148 |          368 |        516 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1058 us
  parse       bench.c       :    302 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    178 us
  link        bench_rcc     :    309 us
  link        bench_rcc     :  91378 us

RCC -O1:
  preprocess  bench.c       :   2600 us
  parse       bench.c       :    414 us
  typecheck   bench.c       :      7 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    176 us
  link        bench_o1      :   1586 us
  link        bench_o1      : 110564 us

RCC -O2:
  preprocess  bench.c       :    704 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    133 us
  link        bench_o2      :    568 us
  link        bench_o2      :  91727 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 572965 us
  parse       sqlite3.c     : 225857 us
  typecheck   sqlite3.c     :  30580 us
  codegen     sqlite3.c     : 208425 us
  link        sqlite3.so    :  22040 us

RCC -O1:
  preprocess  sqlite3.c     : 380478 us
  parse       sqlite3.c     :  84379 us
  typecheck   sqlite3.c     :  34233 us
  opt         sqlite3.c     :  37630 us
  codegen     sqlite3.c     : 155269 us
  link        sqlite3.so    :  23383 us

RCC -O2:
  preprocess  sqlite3.c     : 429370 us
  parse       sqlite3.c     :  96910 us
  typecheck   sqlite3.c     :  17975 us
  opt         sqlite3.c     : 286665 us
  codegen     sqlite3.c     : 254732 us
  link        sqlite3.so    :  32916 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1689 ms |
| RCC -O1   |      1083 ms |
| RCC -O2   |      1021 ms |
| TCC       |       143 ms |
| GCC -O0   |      1381 ms |
| GCC -O2   |     15886 ms |
| Clang -O0 |      1582 ms |
| Clang -O2 |     13328 ms |
