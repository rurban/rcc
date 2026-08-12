# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           62 |          648 |        710 |
| RCC -O1   |           70 |          642 |        712 |
| RCC -O2   |           72 |          649 |        721 |
| TCC       |           50 |          567 |        617 |
| GCC -O0   |           94 |          471 |        565 |
| GCC -O2   |          115 |          262 |        377 |
| Clang -O0 |           51 |          501 |        552 |
| Clang -O2 |          122 |          283 |        405 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1028 us
  parse       bench.c       :    211 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    177 us
  link        bench_rcc     :    117 us
  link        bench_rcc     :  56007 us

RCC -O1:
  preprocess  bench.c       :    746 us
  parse       bench.c       :    184 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    137 us
  link        bench_o1      :    161 us
  link        bench_o1      :  58034 us

RCC -O2:
  preprocess  bench.c       :    811 us
  parse       bench.c       :    181 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    151 us
  link        bench_o2      :    169 us
  link        bench_o2      :  69568 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 296130 us
  parse       sqlite3.c     :  65155 us
  typecheck   sqlite3.c     :  24986 us
  codegen     sqlite3.c     : 108463 us
  link        sqlite3.so    :  16472 us

RCC -O1:
  preprocess  sqlite3.c     : 250419 us
  parse       sqlite3.c     :  62321 us
  typecheck   sqlite3.c     :  14989 us
  opt         sqlite3.c     :  23076 us
  codegen     sqlite3.c     : 105300 us
  link        sqlite3.so    :  16122 us

RCC -O2:
  preprocess  sqlite3.c     : 225655 us
  parse       sqlite3.c     :  46721 us
  typecheck   sqlite3.c     :  12298 us
  opt         sqlite3.c     : 141587 us
  codegen     sqlite3.c     : 101048 us
  link        sqlite3.so    :  16434 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       639 ms |
| RCC -O1   |       898 ms |
| RCC -O2   |      1006 ms |
| TCC       |       120 ms |
| GCC -O0   |      1101 ms |
| GCC -O2   |     10428 ms |
| Clang -O0 |      1050 ms |
| Clang -O2 |     10950 ms |
