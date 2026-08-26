# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          179 |          939 |       1118 |
| RCC -O1   |          166 |         1236 |       1402 |
| RCC -O2   |          154 |          902 |       1056 |
| TCC       |           96 |          672 |        768 |
| GCC -O0   |          115 |          556 |        671 |
| GCC -O2   |          142 |          341 |        483 |
| Clang -O0 |          114 |          564 |        678 |
| Clang -O2 |          194 |          336 |        530 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2493 us
  parse       bench.c       :    980 us
  typecheck   bench.c       :     28 us
  codegen     bench.c       :   1035 us
  link        bench_rcc     :    461 us
  link        bench_rcc     :  95224 us

RCC -O1:
  preprocess  bench.c       :   4048 us
  parse       bench.c       :    322 us
  typecheck   bench.c       :     11 us
  opt         bench.c       :     56 us
  codegen     bench.c       :    474 us
  link        bench_o1      :    207 us
  link        bench_o1      :  96348 us

RCC -O2:
  preprocess  bench.c       :    646 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     31 us
  codegen     bench.c       :    125 us
  link        bench_o2      :    210 us
  link        bench_o2      :  90863 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 434846 us
  parse       sqlite3.c     : 106838 us
  typecheck   sqlite3.c     :  29984 us
  codegen     sqlite3.c     : 166266 us
  link        sqlite3.so    :  18240 us

RCC -O1:
  preprocess  sqlite3.c     : 357432 us
  parse       sqlite3.c     :  99734 us
  typecheck   sqlite3.c     :  35214 us
  opt         sqlite3.c     : 489287 us
  codegen     sqlite3.c     : 239563 us
  link        sqlite3.so    :  28085 us

RCC -O2:
  preprocess  sqlite3.c     : 496536 us
  parse       sqlite3.c     :  97253 us
  typecheck   sqlite3.c     :  26374 us
  opt         sqlite3.c     : 379759 us
  codegen     sqlite3.c     : 306910 us
  link        sqlite3.so    :  22816 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1546 ms |
| RCC -O1   |      1042 ms |
| RCC -O2   |       937 ms |
| TCC       |       113 ms |
| GCC -O0   |      1503 ms |
| GCC -O2   |     14381 ms |
| Clang -O0 |      1468 ms |
| Clang -O2 |     15397 ms |
