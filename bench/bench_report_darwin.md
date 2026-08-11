# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           63 |          638 |        701 |
| RCC -O1   |           59 |          659 |        718 |
| RCC -O2   |           53 |          661 |        714 |
| TCC       |           53 |          620 |        673 |
| GCC -O0   |           90 |          497 |        587 |
| GCC -O2   |          110 |          274 |        384 |
| Clang -O0 |           72 |          477 |        549 |
| Clang -O2 |          100 |          329 |        429 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1840 us
  parse       bench.c       :    400 us
  typecheck   bench.c       :     11 us
  codegen     bench.c       :    318 us
  link        bench_rcc     :    535 us
  link        bench_rcc     :  57362 us

RCC -O1:
  preprocess  bench.c       :    661 us
  parse       bench.c       :    168 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    122 us
  link        bench_o1      :    350 us
  link        bench_o1      :  51953 us

RCC -O2:
  preprocess  bench.c       :    731 us
  parse       bench.c       :    161 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    124 us
  link        bench_o2      :    377 us
  link        bench_o2      :  52615 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 264358 us
  parse       sqlite3.c     :  71820 us
  typecheck   sqlite3.c     :  25496 us
  codegen     sqlite3.c     : 105083 us
  link        sqlite3.so    :  16726 us

RCC -O1:
  preprocess  sqlite3.c     : 262382 us
  parse       sqlite3.c     :  59446 us
  typecheck   sqlite3.c     :  16689 us
  opt         sqlite3.c     :  53014 us
  codegen     sqlite3.c     : 111411 us
  link        sqlite3.so    :  15487 us

RCC -O2:
  preprocess  sqlite3.c     : 239621 us
  parse       sqlite3.c     :  45543 us
  typecheck   sqlite3.c     :  12724 us
  opt         sqlite3.c     : 146611 us
  codegen     sqlite3.c     : 108084 us
  link        sqlite3.so    :  17692 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       977 ms |
| RCC -O1   |       650 ms |
| RCC -O2   |      1067 ms |
| TCC       |       154 ms |
| GCC -O0   |      1256 ms |
| GCC -O2   |     12652 ms |
| Clang -O0 |      1653 ms |
| Clang -O2 |     15644 ms |
