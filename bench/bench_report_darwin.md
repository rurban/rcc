# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           62 |          630 |        692 |
| RCC -O1   |           53 |          630 |        683 |
| RCC -O2   |           51 |          627 |        678 |
| TCC       |           42 |          556 |        598 |
| GCC -O0   |           62 |          486 |        548 |
| GCC -O2   |          127 |          288 |        415 |
| Clang -O0 |           65 |          468 |        533 |
| Clang -O2 |           89 |          284 |        373 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    693 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    142 us
  link        bench_rcc     :     89 us
  link        bench_rcc     :  46938 us

RCC -O1:
  preprocess  bench.c       :    623 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    122 us
  link        bench_o1      :    119 us
  link        bench_o1      :  49575 us

RCC -O2:
  preprocess  bench.c       :    581 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    113 us
  link        bench_o2      :    125 us
  link        bench_o2      :  47496 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 243326 us
  parse       sqlite3.c     :  50101 us
  typecheck   sqlite3.c     :  18086 us
  codegen     sqlite3.c     : 108807 us
  link        sqlite3.so    :  14812 us

RCC -O1:
  preprocess  sqlite3.c     : 216599 us
  parse       sqlite3.c     :  46130 us
  typecheck   sqlite3.c     :  13294 us
  opt         sqlite3.c     :  19973 us
  codegen     sqlite3.c     :  89786 us
  link        sqlite3.so    :  15065 us

RCC -O2:
  preprocess  sqlite3.c     : 208209 us
  parse       sqlite3.c     :  45003 us
  typecheck   sqlite3.c     :  12941 us
  opt         sqlite3.c     : 134889 us
  codegen     sqlite3.c     :  96859 us
  link        sqlite3.so    :  15411 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       672 ms |
| RCC -O1   |       710 ms |
| RCC -O2   |       737 ms |
| TCC       |       102 ms |
| GCC -O0   |      1021 ms |
| GCC -O2   |     10633 ms |
| Clang -O0 |      1326 ms |
| Clang -O2 |     13057 ms |
