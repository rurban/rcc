# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          179 |          748 |        927 |
| RCC -O1   |          178 |          766 |        944 |
| RCC -O2   |          114 |          856 |        970 |
| TCC       |          102 |          677 |        779 |
| GCC -O0   |          130 |          504 |        634 |
| GCC -O2   |          120 |          266 |        386 |
| Clang -O0 |           51 |          481 |        532 |
| Clang -O2 |           95 |          289 |        384 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1178 us
  parse       bench.c       :    267 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    187 us
  link        bench_rcc     :    272 us
  link        bench_rcc     :  72441 us

RCC -O1:
  preprocess  bench.c       :    682 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    120 us
  link        bench_o1      :    172 us
  link        bench_o1      :  66127 us

RCC -O2:
  preprocess  bench.c       :    628 us
  parse       bench.c       :    160 us
  typecheck   bench.c       :     26 us
  opt         bench.c       :     32 us
  codegen     bench.c       :    118 us
  link        bench_o2      :    200 us
  link        bench_o2      :  81028 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 308945 us
  parse       sqlite3.c     :  73311 us
  typecheck   sqlite3.c     :  17629 us
  codegen     sqlite3.c     : 130538 us
  link        sqlite3.so    :  19422 us

RCC -O1:
  preprocess  sqlite3.c     : 391571 us
  parse       sqlite3.c     :  71084 us
  typecheck   sqlite3.c     :  15601 us
  opt         sqlite3.c     :  42063 us
  codegen     sqlite3.c     : 196059 us
  link        sqlite3.so    :  18748 us

RCC -O2:
  preprocess  sqlite3.c     : 451582 us
  parse       sqlite3.c     : 126068 us
  typecheck   sqlite3.c     :  21712 us
  opt         sqlite3.c     : 234838 us
  codegen     sqlite3.c     : 371227 us
  link        sqlite3.so    :  22066 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       770 ms |
| RCC -O1   |       673 ms |
| RCC -O2   |      1058 ms |
| TCC       |       127 ms |
| GCC -O0   |      1154 ms |
| GCC -O2   |     11718 ms |
| Clang -O0 |      1219 ms |
| Clang -O2 |     10543 ms |
