# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           60 |          710 |        770 |
| RCC -O1   |           91 |          680 |        771 |
| RCC -O2   |           70 |          643 |        713 |
| TCC       |           53 |          544 |        597 |
| GCC -O0   |           75 |          470 |        545 |
| GCC -O2   |          106 |          285 |        391 |
| Clang -O0 |           62 |          452 |        514 |
| Clang -O2 |          106 |          284 |        390 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1227 us
  parse       bench.c       :    198 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    248 us
  link        bench_rcc     :    182 us
  link        bench_rcc     :  67143 us

RCC -O1:
  preprocess  bench.c       :    693 us
  parse       bench.c       :    160 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    148 us
  link        bench_o1      :    145 us
  link        bench_o1      :  50865 us

RCC -O2:
  preprocess  bench.c       :    649 us
  parse       bench.c       :    203 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    174 us
  link        bench_o2      :    126 us
  link        bench_o2      :  48988 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 253927 us
  parse       sqlite3.c     :  51915 us
  typecheck   sqlite3.c     :  12815 us
  codegen     sqlite3.c     : 136780 us
  link        sqlite3.so    :  19507 us

RCC -O1:
  preprocess  sqlite3.c     : 222856 us
  parse       sqlite3.c     :  50460 us
  typecheck   sqlite3.c     :  12593 us
  opt         sqlite3.c     : 125392 us
  codegen     sqlite3.c     : 117248 us
  link        sqlite3.so    :  20478 us

RCC -O2:
  preprocess  sqlite3.c     : 264116 us
  parse       sqlite3.c     :  52486 us
  typecheck   sqlite3.c     :  13625 us
  opt         sqlite3.c     : 129470 us
  codegen     sqlite3.c     : 124660 us
  link        sqlite3.so    :  15803 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       611 ms |
| RCC -O1   |       796 ms |
| RCC -O2   |       701 ms |
| TCC       |       104 ms |
| GCC -O0   |      1033 ms |
| GCC -O2   |     11705 ms |
| Clang -O0 |      1160 ms |
| Clang -O2 |      9796 ms |
