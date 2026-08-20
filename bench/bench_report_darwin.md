# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           55 |          648 |        703 |
| RCC -O1   |           58 |          670 |        728 |
| RCC -O2   |           70 |          657 |        727 |
| TCC       |           58 |          628 |        686 |
| GCC -O0   |           71 |          546 |        617 |
| GCC -O2   |          130 |          328 |        458 |
| Clang -O0 |           81 |          563 |        644 |
| Clang -O2 |          100 |          345 |        445 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    863 us
  parse       bench.c       :    153 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    245 us
  link        bench_rcc     :    380 us
  link        bench_rcc     :  83543 us

RCC -O1:
  preprocess  bench.c       :    729 us
  parse       bench.c       :    132 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    139 us
  link        bench_o1      :    162 us
  link        bench_o1      :  58286 us

RCC -O2:
  preprocess  bench.c       :    999 us
  parse       bench.c       :    144 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    161 us
  link        bench_o2      :    364 us
  link        bench_o2      :  71578 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 253914 us
  parse       sqlite3.c     :  58705 us
  typecheck   sqlite3.c     :  13349 us
  codegen     sqlite3.c     : 107370 us
  link        sqlite3.so    :  16578 us

RCC -O1:
  preprocess  sqlite3.c     : 214669 us
  parse       sqlite3.c     :  52964 us
  typecheck   sqlite3.c     :  13790 us
  opt         sqlite3.c     : 151557 us
  codegen     sqlite3.c     : 100865 us
  link        sqlite3.so    :  16419 us

RCC -O2:
  preprocess  sqlite3.c     : 200666 us
  parse       sqlite3.c     :  48376 us
  typecheck   sqlite3.c     :  13432 us
  opt         sqlite3.c     : 136845 us
  codegen     sqlite3.c     : 110166 us
  link        sqlite3.so    :  16145 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       707 ms |
| RCC -O1   |       901 ms |
| RCC -O2   |       810 ms |
| TCC       |       106 ms |
| GCC -O0   |      1128 ms |
| GCC -O2   |     11633 ms |
| Clang -O0 |      1168 ms |
| Clang -O2 |     11047 ms |
