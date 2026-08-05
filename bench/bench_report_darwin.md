# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          148 |          738 |        886 |
| RCC -O1   |          111 |          758 |        869 |
| RCC -O2   |           91 |          793 |        884 |
| TCC       |           79 |          625 |        704 |
| GCC -O0   |          181 |          532 |        713 |
| GCC -O2   |          194 |          387 |        581 |
| Clang -O0 |          146 |          653 |        799 |
| Clang -O2 |          162 |          348 |        510 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    780 us
  parse       bench.c       :    164 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    125 us
  link        bench_rcc     :    321 us
  link        bench_rcc     :  69760 us

RCC -O1:
  preprocess  bench.c       :    629 us
  parse       bench.c       :    116 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    110 us
  link        bench_o1      :    146 us
  link        bench_o1      :  69569 us

RCC -O2:
  preprocess  bench.c       :    613 us
  parse       bench.c       :    133 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    155 us
  link        bench_o2      :    206 us
  link        bench_o2      :  62599 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 342933 us
  parse       sqlite3.c     : 179286 us
  typecheck   sqlite3.c     :  18696 us
  codegen     sqlite3.c     : 189712 us
  link        sqlite3.so    :  16841 us

RCC -O1:
  preprocess  sqlite3.c     : 358686 us
  parse       sqlite3.c     : 129989 us
  typecheck   sqlite3.c     :  27186 us
  opt         sqlite3.c     :  34233 us
  codegen     sqlite3.c     : 206090 us
  link        sqlite3.so    :  29580 us

RCC -O2:
  preprocess  sqlite3.c     : 424853 us
  parse       sqlite3.c     : 118609 us
  typecheck   sqlite3.c     :  30305 us
  opt         sqlite3.c     : 327046 us
  codegen     sqlite3.c     : 158571 us
  link        sqlite3.so    :  21933 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1217 ms |
| RCC -O1   |      1095 ms |
| RCC -O2   |      1736 ms |
| TCC       |       220 ms |
| GCC -O0   |      2471 ms |
| GCC -O2   |     17141 ms |
| Clang -O0 |      1655 ms |
| Clang -O2 |     16606 ms |
