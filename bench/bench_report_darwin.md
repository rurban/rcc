# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          154 |          678 |        832 |
| RCC -O1   |           97 |          829 |        926 |
| RCC -O2   |          133 |          738 |        871 |
| TCC       |           62 |          558 |        620 |
| GCC -O0   |          101 |          480 |        581 |
| GCC -O2   |          150 |          289 |        439 |
| Clang -O0 |           63 |          502 |        565 |
| Clang -O2 |          110 |          314 |        424 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1192 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    115 us
  link        bench_rcc     :    218 us
  link        bench_rcc     :  46552 us

RCC -O1:
  preprocess  bench.c       :    910 us
  parse       bench.c       :    150 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    123 us
  link        bench_o1      :    137 us
  link        bench_o1      :  54828 us

RCC -O2:
  preprocess  bench.c       :    619 us
  parse       bench.c       :    174 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    118 us
  link        bench_o2      :    205 us
  link        bench_o2      :  54464 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 280325 us
  parse       sqlite3.c     : 198859 us
  typecheck   sqlite3.c     :  18331 us
  codegen     sqlite3.c     : 122514 us
  link        sqlite3.so    :  18848 us

RCC -O1:
  preprocess  sqlite3.c     : 284042 us
  parse       sqlite3.c     :  85563 us
  typecheck   sqlite3.c     :  22948 us
  opt         sqlite3.c     : 207884 us
  codegen     sqlite3.c     : 153137 us
  link        sqlite3.so    :  16741 us

RCC -O2:
  preprocess  sqlite3.c     : 313717 us
  parse       sqlite3.c     :  61648 us
  typecheck   sqlite3.c     :  11183 us
  opt         sqlite3.c     : 240613 us
  codegen     sqlite3.c     : 163052 us
  link        sqlite3.so    :  17878 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1488 ms |
| RCC -O1   |       969 ms |
| RCC -O2   |       807 ms |
| TCC       |       146 ms |
| GCC -O0   |      1399 ms |
| GCC -O2   |     14327 ms |
| Clang -O0 |      2008 ms |
| Clang -O2 |     15939 ms |
