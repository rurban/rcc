# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           71 |          628 |        699 |
| RCC -O1   |           66 |          641 |        707 |
| RCC -O2   |           77 |          632 |        709 |
| TCC       |           50 |          553 |        603 |
| GCC -O0   |          135 |          486 |        621 |
| GCC -O2   |          298 |          300 |        598 |
| Clang -O0 |           60 |          468 |        528 |
| Clang -O2 |           98 |          281 |        379 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    964 us
  parse       bench.c       :    223 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    174 us
  link        bench_rcc     :    114 us
  link        bench_rcc     :  69348 us

RCC -O1:
  preprocess  bench.c       :   1029 us
  parse       bench.c       :    127 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    158 us
  link        bench_o1      :    604 us
  link        bench_o1      :  90793 us

RCC -O2:
  preprocess  bench.c       :    655 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    123 us
  link        bench_o2      :    118 us
  link        bench_o2      :  60092 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 278016 us
  parse       sqlite3.c     : 143627 us
  typecheck   sqlite3.c     :  32569 us
  codegen     sqlite3.c     : 233185 us
  link        sqlite3.so    :  18001 us

RCC -O1:
  preprocess  sqlite3.c     : 305466 us
  parse       sqlite3.c     :  57643 us
  typecheck   sqlite3.c     :  13768 us
  opt         sqlite3.c     : 136316 us
  codegen     sqlite3.c     :  98960 us
  link        sqlite3.so    :  15279 us

RCC -O2:
  preprocess  sqlite3.c     : 228252 us
  parse       sqlite3.c     :  47837 us
  typecheck   sqlite3.c     :  12672 us
  opt         sqlite3.c     : 136684 us
  codegen     sqlite3.c     : 107999 us
  link        sqlite3.so    :  16300 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       861 ms |
| RCC -O1   |       809 ms |
| RCC -O2   |       785 ms |
| TCC       |       106 ms |
| GCC -O0   |      1035 ms |
| GCC -O2   |     11722 ms |
| Clang -O0 |      1096 ms |
| Clang -O2 |     10131 ms |
