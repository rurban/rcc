# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           73 |          717 |        790 |
| RCC -O1   |          108 |          666 |        774 |
| RCC -O2   |           72 |          694 |        766 |
| TCC       |           67 |          590 |        657 |
| GCC -O0   |           77 |          500 |        577 |
| GCC -O2   |          160 |          298 |        458 |
| Clang -O0 |           85 |          509 |        594 |
| Clang -O2 |          116 |          321 |        437 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1860 us
  parse       bench.c       :    306 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    292 us
  link        bench_rcc     :    318 us
  link        bench_rcc     :  76728 us

RCC -O1:
  preprocess  bench.c       :   1638 us
  parse       bench.c       :    321 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    636 us
  link        bench_o1      :    270 us
  link        bench_o1      : 106777 us

RCC -O2:
  preprocess  bench.c       :    846 us
  parse       bench.c       :    297 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    223 us
  link        bench_o2      :    552 us
  link        bench_o2      :  88800 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 360328 us
  parse       sqlite3.c     : 111248 us
  typecheck   sqlite3.c     :  12325 us
  codegen     sqlite3.c     : 141438 us
  link        sqlite3.so    :  21588 us

RCC -O1:
  preprocess  sqlite3.c     : 352006 us
  parse       sqlite3.c     :  66519 us
  typecheck   sqlite3.c     :  13764 us
  opt         sqlite3.c     : 186116 us
  codegen     sqlite3.c     : 164270 us
  link        sqlite3.so    :  18244 us

RCC -O2:
  preprocess  sqlite3.c     : 212392 us
  parse       sqlite3.c     :  76916 us
  typecheck   sqlite3.c     :  16944 us
  opt         sqlite3.c     : 214656 us
  codegen     sqlite3.c     : 169151 us
  link        sqlite3.so    :  17339 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       877 ms |
| RCC -O1   |       846 ms |
| RCC -O2   |       820 ms |
| TCC       |       103 ms |
| GCC -O0   |      1127 ms |
| GCC -O2   |     14565 ms |
| Clang -O0 |      1981 ms |
| Clang -O2 |     17308 ms |
