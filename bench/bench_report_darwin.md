# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          164 |          724 |        888 |
| RCC -O1   |           99 |          711 |        810 |
| RCC -O2   |          130 |          753 |        883 |
| TCC       |           50 |          737 |        787 |
| GCC -O0   |          109 |          551 |        660 |
| GCC -O2   |          197 |          337 |        534 |
| Clang -O0 |           86 |          542 |        628 |
| Clang -O2 |          140 |          315 |        455 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    895 us
  parse       bench.c       :    221 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    264 us
  link        bench_rcc     :    523 us
  link        bench_rcc     :  73309 us

RCC -O1:
  preprocess  bench.c       :    818 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    148 us
  link        bench_o1      :    504 us
  link        bench_o1      :  80944 us

RCC -O2:
  preprocess  bench.c       :    830 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    148 us
  link        bench_o2      :    441 us
  link        bench_o2      :  65126 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 341085 us
  parse       sqlite3.c     : 283253 us
  typecheck   sqlite3.c     :  55667 us
  codegen     sqlite3.c     : 179306 us
  link        sqlite3.so    :  19408 us

RCC -O1:
  preprocess  sqlite3.c     : 378367 us
  parse       sqlite3.c     :  74432 us
  typecheck   sqlite3.c     :  26414 us
  opt         sqlite3.c     : 282204 us
  codegen     sqlite3.c     : 222536 us
  link        sqlite3.so    :  29918 us

RCC -O2:
  preprocess  sqlite3.c     : 328042 us
  parse       sqlite3.c     :  76955 us
  typecheck   sqlite3.c     :  13321 us
  opt         sqlite3.c     : 248231 us
  codegen     sqlite3.c     : 178792 us
  link        sqlite3.so    :  30620 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1228 ms |
| RCC -O1   |       986 ms |
| RCC -O2   |      1007 ms |
| TCC       |       125 ms |
| GCC -O0   |      1511 ms |
| GCC -O2   |     13821 ms |
| Clang -O0 |      1496 ms |
| Clang -O2 |     12831 ms |
