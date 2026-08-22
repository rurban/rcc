# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           47 |          593 |        640 |
| RCC -O1   |           48 |          594 |        642 |
| RCC -O2   |           60 |          593 |        653 |
| TCC       |           39 |          513 |        552 |
| GCC -O0   |           60 |          439 |        499 |
| GCC -O2   |          114 |          309 |        423 |
| Clang -O0 |           57 |          433 |        490 |
| Clang -O2 |           80 |          262 |        342 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    709 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    154 us
  link        bench_rcc     :    123 us
  link        bench_rcc     :  47365 us

RCC -O1:
  preprocess  bench.c       :    766 us
  parse       bench.c       :    147 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    146 us
  link        bench_o1      :     69 us
  link        bench_o1      :  44559 us

RCC -O2:
  preprocess  bench.c       :    598 us
  parse       bench.c       :    141 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    128 us
  link        bench_o2      :    138 us
  link        bench_o2      :  44942 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 194740 us
  parse       sqlite3.c     :  46329 us
  typecheck   sqlite3.c     :  11533 us
  codegen     sqlite3.c     :  85334 us
  link        sqlite3.so    :  13663 us

RCC -O1:
  preprocess  sqlite3.c     : 175479 us
  parse       sqlite3.c     :  43427 us
  typecheck   sqlite3.c     :  11539 us
  opt         sqlite3.c     : 138915 us
  codegen     sqlite3.c     :  98469 us
  link        sqlite3.so    :  21938 us

RCC -O2:
  preprocess  sqlite3.c     : 199604 us
  parse       sqlite3.c     :  48232 us
  typecheck   sqlite3.c     :  15544 us
  opt         sqlite3.c     : 129121 us
  codegen     sqlite3.c     :  84927 us
  link        sqlite3.so    :  13778 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       566 ms |
| RCC -O1   |       634 ms |
| RCC -O2   |       649 ms |
| TCC       |        98 ms |
| GCC -O0   |       975 ms |
| GCC -O2   |      8666 ms |
| Clang -O0 |      1142 ms |
| Clang -O2 |      9206 ms |
