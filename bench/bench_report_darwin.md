# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           67 |          645 |        712 |
| RCC -O1   |           57 |          641 |        698 |
| RCC -O2   |           65 |          642 |        707 |
| TCC       |           42 |          557 |        599 |
| GCC -O0   |           67 |          472 |        539 |
| GCC -O2   |          123 |          284 |        407 |
| Clang -O0 |           57 |          468 |        525 |
| Clang -O2 |           88 |          283 |        371 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    696 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    185 us
  link        bench_rcc     :    108 us
  link        bench_rcc     :  50352 us

RCC -O1:
  preprocess  bench.c       :    611 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    130 us
  link        bench_o1      :    158 us
  link        bench_o1      :  49750 us

RCC -O2:
  preprocess  bench.c       :    630 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    138 us
  link        bench_o2      :    108 us
  link        bench_o2      :  47905 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 222411 us
  parse       sqlite3.c     :  52584 us
  typecheck   sqlite3.c     :  11147 us
  codegen     sqlite3.c     : 104600 us
  link        sqlite3.so    :  14544 us

RCC -O1:
  preprocess  sqlite3.c     : 235519 us
  parse       sqlite3.c     :  54252 us
  typecheck   sqlite3.c     :  13659 us
  opt         sqlite3.c     : 158765 us
  codegen     sqlite3.c     : 107472 us
  link        sqlite3.so    :  14185 us

RCC -O2:
  preprocess  sqlite3.c     : 189758 us
  parse       sqlite3.c     :  49047 us
  typecheck   sqlite3.c     :  11113 us
  opt         sqlite3.c     : 140258 us
  codegen     sqlite3.c     : 106047 us
  link        sqlite3.so    :  20219 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       587 ms |
| RCC -O1   |       717 ms |
| RCC -O2   |       712 ms |
| TCC       |        96 ms |
| GCC -O0   |      1002 ms |
| GCC -O2   |     10109 ms |
| Clang -O0 |      1032 ms |
| Clang -O2 |      9623 ms |
