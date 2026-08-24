# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          121 |          830 |        951 |
| RCC -O1   |          101 |          700 |        801 |
| RCC -O2   |           90 |          672 |        762 |
| TCC       |           43 |          550 |        593 |
| GCC -O0   |          102 |          475 |        577 |
| GCC -O2   |          123 |          334 |        457 |
| Clang -O0 |           89 |          523 |        612 |
| Clang -O2 |          214 |          387 |        601 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1178 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    126 us
  link        bench_rcc     :    171 us
  link        bench_rcc     :  79700 us

RCC -O1:
  preprocess  bench.c       :    724 us
  parse       bench.c       :    131 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    141 us
  link        bench_o1      :    557 us
  link        bench_o1      :  66377 us

RCC -O2:
  preprocess  bench.c       :   1597 us
  parse       bench.c       :    276 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     44 us
  codegen     bench.c       :    249 us
  link        bench_o2      :    644 us
  link        bench_o2      :  78898 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 328024 us
  parse       sqlite3.c     : 175109 us
  typecheck   sqlite3.c     :  24596 us
  codegen     sqlite3.c     : 142764 us
  link        sqlite3.so    :  17715 us

RCC -O1:
  preprocess  sqlite3.c     : 317443 us
  parse       sqlite3.c     :  75555 us
  typecheck   sqlite3.c     :  19201 us
  opt         sqlite3.c     : 272598 us
  codegen     sqlite3.c     : 154465 us
  link        sqlite3.so    :  21212 us

RCC -O2:
  preprocess  sqlite3.c     : 315142 us
  parse       sqlite3.c     :  68441 us
  typecheck   sqlite3.c     :  15419 us
  opt         sqlite3.c     : 216064 us
  codegen     sqlite3.c     : 199380 us
  link        sqlite3.so    :  27477 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1466 ms |
| RCC -O1   |      1376 ms |
| RCC -O2   |      1501 ms |
| TCC       |       223 ms |
| GCC -O0   |      1613 ms |
| GCC -O2   |     14250 ms |
| Clang -O0 |      1861 ms |
| Clang -O2 |     12347 ms |
