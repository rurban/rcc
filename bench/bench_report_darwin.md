# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           68 |          731 |        799 |
| RCC -O1   |          114 |          696 |        810 |
| RCC -O2   |          129 |          765 |        894 |
| TCC       |          120 |          682 |        802 |
| GCC -O0   |          147 |          580 |        727 |
| GCC -O2   |          175 |          310 |        485 |
| Clang -O0 |          154 |          492 |        646 |
| Clang -O2 |          104 |          265 |        369 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    853 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    134 us
  link        bench_rcc     :    213 us
  link        bench_rcc     :  60066 us

RCC -O1:
  preprocess  bench.c       :    627 us
  parse       bench.c       :    133 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    118 us
  link        bench_o1      :    183 us
  link        bench_o1      :  60328 us

RCC -O2:
  preprocess  bench.c       :    623 us
  parse       bench.c       :    127 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    123 us
  link        bench_o2      :    193 us
  link        bench_o2      :  58119 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 320077 us
  parse       sqlite3.c     :  55941 us
  typecheck   sqlite3.c     :  15665 us
  codegen     sqlite3.c     : 101386 us
  link        sqlite3.so    :  15697 us

RCC -O1:
  preprocess  sqlite3.c     : 233692 us
  parse       sqlite3.c     :  55226 us
  typecheck   sqlite3.c     :  15504 us
  opt         sqlite3.c     : 151533 us
  codegen     sqlite3.c     : 118017 us
  link        sqlite3.so    :  16914 us

RCC -O2:
  preprocess  sqlite3.c     : 242761 us
  parse       sqlite3.c     :  56958 us
  typecheck   sqlite3.c     :  15728 us
  opt         sqlite3.c     : 170200 us
  codegen     sqlite3.c     : 106091 us
  link        sqlite3.so    :  17517 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       687 ms |
| RCC -O1   |       711 ms |
| RCC -O2   |       762 ms |
| TCC       |       101 ms |
| GCC -O0   |      1143 ms |
| GCC -O2   |     11592 ms |
| Clang -O0 |      1198 ms |
| Clang -O2 |     12341 ms |
