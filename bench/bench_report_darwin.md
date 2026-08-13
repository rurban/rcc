# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           57 |          624 |        681 |
| RCC -O1   |           54 |          630 |        684 |
| RCC -O2   |           60 |          628 |        688 |
| TCC       |           52 |          555 |        607 |
| GCC -O0   |           65 |          469 |        534 |
| GCC -O2   |          111 |          283 |        394 |
| Clang -O0 |           54 |          479 |        533 |
| Clang -O2 |           95 |          295 |        390 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    685 us
  parse       bench.c       :    117 us
  typecheck   bench.c       :     14 us
  codegen     bench.c       :    143 us
  link        bench_rcc     :    229 us
  link        bench_rcc     :  52031 us

RCC -O1:
  preprocess  bench.c       :    575 us
  parse       bench.c       :    119 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    115 us
  link        bench_o1      :    120 us
  link        bench_o1      :  47026 us

RCC -O2:
  preprocess  bench.c       :    614 us
  parse       bench.c       :    118 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    111 us
  link        bench_o2      :    390 us
  link        bench_o2      :  47924 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 278577 us
  parse       sqlite3.c     : 103577 us
  typecheck   sqlite3.c     :  30146 us
  codegen     sqlite3.c     : 105409 us
  link        sqlite3.so    :  16458 us

RCC -O1:
  preprocess  sqlite3.c     : 239450 us
  parse       sqlite3.c     :  47753 us
  typecheck   sqlite3.c     :  13824 us
  opt         sqlite3.c     : 132227 us
  codegen     sqlite3.c     :  97268 us
  link        sqlite3.so    :  15629 us

RCC -O2:
  preprocess  sqlite3.c     : 210458 us
  parse       sqlite3.c     :  44019 us
  typecheck   sqlite3.c     :  12484 us
  opt         sqlite3.c     : 138637 us
  codegen     sqlite3.c     :  93755 us
  link        sqlite3.so    :  15578 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       809 ms |
| RCC -O1   |       869 ms |
| RCC -O2   |       829 ms |
| TCC       |       117 ms |
| GCC -O0   |      1275 ms |
| GCC -O2   |     11910 ms |
| Clang -O0 |      1169 ms |
| Clang -O2 |      9992 ms |
