# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           62 |          621 |        683 |
| RCC -O1   |           48 |          615 |        663 |
| RCC -O2   |           48 |          616 |        664 |
| TCC       |           39 |          532 |        571 |
| GCC -O0   |           56 |          447 |        503 |
| GCC -O2   |           87 |          270 |        357 |
| Clang -O0 |           50 |          446 |        496 |
| Clang -O2 |           80 |          273 |        353 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    599 us
  parse       bench.c       :    114 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    110 us
  link        bench_rcc     :     97 us
  link        bench_rcc     :  45085 us

RCC -O1:
  preprocess  bench.c       :    610 us
  parse       bench.c       :    185 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    128 us
  link        bench_o1      :    126 us
  link        bench_o1      :  46775 us

RCC -O2:
  preprocess  bench.c       :    550 us
  parse       bench.c       :    116 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    108 us
  link        bench_o2      :    162 us
  link        bench_o2      :  41567 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 182959 us
  parse       sqlite3.c     :  46003 us
  typecheck   sqlite3.c     :  11951 us
  codegen     sqlite3.c     :  92408 us
  link        sqlite3.so    :  15024 us

RCC -O1:
  preprocess  sqlite3.c     : 183095 us
  parse       sqlite3.c     :  45827 us
  typecheck   sqlite3.c     :  11947 us
  opt         sqlite3.c     : 133279 us
  codegen     sqlite3.c     :  89626 us
  link        sqlite3.so    :  13670 us

RCC -O2:
  preprocess  sqlite3.c     : 183588 us
  parse       sqlite3.c     :  44429 us
  typecheck   sqlite3.c     :  12561 us
  opt         sqlite3.c     : 133526 us
  codegen     sqlite3.c     :  85930 us
  link        sqlite3.so    :  15542 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       662 ms |
| RCC -O1   |       669 ms |
| RCC -O2   |       665 ms |
| TCC       |        85 ms |
| GCC -O0   |       953 ms |
| GCC -O2   |      9183 ms |
| Clang -O0 |       974 ms |
| Clang -O2 |      9253 ms |
