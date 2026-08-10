# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          121 |          811 |        932 |
| RCC -O1   |          106 |          801 |        907 |
| RCC -O2   |          128 |          830 |        958 |
| TCC       |           96 |          692 |        788 |
| GCC -O0   |          139 |          583 |        722 |
| GCC -O2   |          183 |          375 |        558 |
| Clang -O0 |           99 |          691 |        790 |
| Clang -O2 |          170 |          351 |        521 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1115 us
  parse       bench.c       :    144 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    158 us
  link        bench_rcc     :    117 us
  link        bench_rcc     :  53731 us

RCC -O1:
  preprocess  bench.c       :   1650 us
  parse       bench.c       :    404 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    182 us
  link        bench_o1      :    231 us
  link        bench_o1      :  52386 us

RCC -O2:
  preprocess  bench.c       :    786 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    126 us
  link        bench_o2      :     78 us
  link        bench_o2      :  68532 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 389099 us
  parse       sqlite3.c     : 196135 us
  typecheck   sqlite3.c     :  54270 us
  codegen     sqlite3.c     : 169268 us
  link        sqlite3.so    :  19414 us

RCC -O1:
  preprocess  sqlite3.c     : 289953 us
  parse       sqlite3.c     :  57190 us
  typecheck   sqlite3.c     :  12885 us
  opt         sqlite3.c     :  19200 us
  codegen     sqlite3.c     : 160508 us
  link        sqlite3.so    :  15923 us

RCC -O2:
  preprocess  sqlite3.c     : 334083 us
  parse       sqlite3.c     :  94543 us
  typecheck   sqlite3.c     :  26113 us
  opt         sqlite3.c     : 206193 us
  codegen     sqlite3.c     : 152748 us
  link        sqlite3.so    :  16352 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1593 ms |
| RCC -O1   |      1132 ms |
| RCC -O2   |      1426 ms |
| TCC       |       166 ms |
| GCC -O0   |      2216 ms |
| GCC -O2   |     18366 ms |
| Clang -O0 |      2173 ms |
| Clang -O2 |     15164 ms |
