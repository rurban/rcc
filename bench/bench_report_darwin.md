# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           88 |          627 |        715 |
| RCC -O1   |           56 |          644 |        700 |
| RCC -O2   |           59 |          657 |        716 |
| TCC       |           55 |          581 |        636 |
| GCC -O0   |           76 |          511 |        587 |
| GCC -O2   |          160 |          290 |        450 |
| Clang -O0 |           58 |          472 |        530 |
| Clang -O2 |           93 |          310 |        403 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    825 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    132 us
  link        bench_rcc     :    138 us
  link        bench_rcc     :  56295 us

RCC -O1:
  preprocess  bench.c       :    636 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    157 us
  link        bench_o1      :    168 us
  link        bench_o1      :  50845 us

RCC -O2:
  preprocess  bench.c       :    671 us
  parse       bench.c       :    142 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    122 us
  link        bench_o2      :    131 us
  link        bench_o2      :  53212 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 262538 us
  parse       sqlite3.c     : 160879 us
  typecheck   sqlite3.c     :  26809 us
  codegen     sqlite3.c     : 144799 us
  link        sqlite3.so    :  17677 us

RCC -O1:
  preprocess  sqlite3.c     : 264022 us
  parse       sqlite3.c     :  58391 us
  typecheck   sqlite3.c     :  13842 us
  opt         sqlite3.c     :  20659 us
  codegen     sqlite3.c     : 131493 us
  link        sqlite3.so    :  19563 us

RCC -O2:
  preprocess  sqlite3.c     : 319899 us
  parse       sqlite3.c     :  50015 us
  typecheck   sqlite3.c     :  12984 us
  opt         sqlite3.c     : 165563 us
  codegen     sqlite3.c     : 116661 us
  link        sqlite3.so    :  16290 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       847 ms |
| RCC -O1   |       699 ms |
| RCC -O2   |       705 ms |
| TCC       |        98 ms |
| GCC -O0   |      1025 ms |
| GCC -O2   |     10146 ms |
| Clang -O0 |      1293 ms |
| Clang -O2 |     14613 ms |
