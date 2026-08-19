# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           77 |          641 |        718 |
| RCC -O1   |           64 |          656 |        720 |
| RCC -O2   |           74 |          644 |        718 |
| TCC       |           53 |          556 |        609 |
| GCC -O0   |           81 |          467 |        548 |
| GCC -O2   |          101 |          285 |        386 |
| Clang -O0 |           56 |          467 |        523 |
| Clang -O2 |           88 |          281 |        369 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    794 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    190 us
  link        bench_rcc     :    279 us
  link        bench_rcc     :  66310 us

RCC -O1:
  preprocess  bench.c       :    684 us
  parse       bench.c       :    140 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    139 us
  link        bench_o1      :    282 us
  link        bench_o1      :  55077 us

RCC -O2:
  preprocess  bench.c       :    585 us
  parse       bench.c       :    127 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    123 us
  link        bench_o2      :    320 us
  link        bench_o2      :  56854 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 259344 us
  parse       sqlite3.c     :  84154 us
  typecheck   sqlite3.c     :  24561 us
  codegen     sqlite3.c     : 163315 us
  link        sqlite3.so    :  19367 us

RCC -O1:
  preprocess  sqlite3.c     : 322046 us
  parse       sqlite3.c     :  66751 us
  typecheck   sqlite3.c     :  14717 us
  opt         sqlite3.c     : 195816 us
  codegen     sqlite3.c     : 142169 us
  link        sqlite3.so    :  18288 us

RCC -O2:
  preprocess  sqlite3.c     : 305450 us
  parse       sqlite3.c     :  57300 us
  typecheck   sqlite3.c     :  13299 us
  opt         sqlite3.c     : 141241 us
  codegen     sqlite3.c     :  98592 us
  link        sqlite3.so    :  15868 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       610 ms |
| RCC -O1   |       750 ms |
| RCC -O2   |       834 ms |
| TCC       |       110 ms |
| GCC -O0   |      1225 ms |
| GCC -O2   |      9880 ms |
| Clang -O0 |      1104 ms |
| Clang -O2 |     10934 ms |
