# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           60 |          627 |        687 |
| RCC -O1   |           58 |          606 |        664 |
| RCC -O2   |           58 |          623 |        681 |
| TCC       |           38 |          540 |        578 |
| GCC -O0   |           69 |          478 |        547 |
| GCC -O2   |          112 |          283 |        395 |
| Clang -O0 |           58 |          476 |        534 |
| Clang -O2 |           93 |          267 |        360 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    618 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    109 us
  link        bench_rcc     :    217 us
  link        bench_rcc     :  47383 us

RCC -O1:
  preprocess  bench.c       :    664 us
  parse       bench.c       :    113 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    108 us
  link        bench_o1      :    108 us
  link        bench_o1      :  43091 us

RCC -O2:
  preprocess  bench.c       :    515 us
  parse       bench.c       :    104 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    105 us
  link        bench_o2      :     91 us
  link        bench_o2      :  41554 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 256360 us
  parse       sqlite3.c     :  46476 us
  typecheck   sqlite3.c     :  14271 us
  codegen     sqlite3.c     :  88894 us
  link        sqlite3.so    :  15917 us

RCC -O1:
  preprocess  sqlite3.c     : 222360 us
  parse       sqlite3.c     :  49818 us
  typecheck   sqlite3.c     :  11943 us
  opt         sqlite3.c     :  18642 us
  codegen     sqlite3.c     : 105834 us
  link        sqlite3.so    :  13903 us

RCC -O2:
  preprocess  sqlite3.c     : 240657 us
  parse       sqlite3.c     :  51608 us
  typecheck   sqlite3.c     :  15088 us
  opt         sqlite3.c     : 153217 us
  codegen     sqlite3.c     : 132789 us
  link        sqlite3.so    :  15787 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       662 ms |
| RCC -O1   |       576 ms |
| RCC -O2   |       776 ms |
| TCC       |       115 ms |
| GCC -O0   |      1017 ms |
| GCC -O2   |     10243 ms |
| Clang -O0 |      1020 ms |
| Clang -O2 |     10047 ms |
