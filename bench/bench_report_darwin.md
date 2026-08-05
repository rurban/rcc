# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           98 |          739 |        837 |
| RCC -O1   |           65 |          768 |        833 |
| RCC -O2   |           81 |          767 |        848 |
| TCC       |           70 |          679 |        749 |
| GCC -O0   |           98 |          598 |        696 |
| GCC -O2   |          285 |          355 |        640 |
| Clang -O0 |          153 |          568 |        721 |
| Clang -O2 |          282 |          348 |        630 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1007 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    121 us
  link        bench_rcc     :    212 us
  link        bench_rcc     :  61051 us

RCC -O1:
  preprocess  bench.c       :    573 us
  parse       bench.c       :    118 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    107 us
  link        bench_o1      :    333 us
  link        bench_o1      :  76721 us

RCC -O2:
  preprocess  bench.c       :    592 us
  parse       bench.c       :    285 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     46 us
  codegen     bench.c       :    236 us
  link        bench_o2      :    265 us
  link        bench_o2      :  65857 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 335711 us
  parse       sqlite3.c     : 173601 us
  typecheck   sqlite3.c     :  27226 us
  codegen     sqlite3.c     : 172647 us
  link        sqlite3.so    :  17244 us

RCC -O1:
  preprocess  sqlite3.c     : 300519 us
  parse       sqlite3.c     : 101293 us
  typecheck   sqlite3.c     :  24126 us
  opt         sqlite3.c     :  39967 us
  codegen     sqlite3.c     : 137732 us
  link        sqlite3.so    :  20796 us

RCC -O2:
  preprocess  sqlite3.c     : 329075 us
  parse       sqlite3.c     :  61856 us
  typecheck   sqlite3.c     :  16497 us
  opt         sqlite3.c     : 194241 us
  codegen     sqlite3.c     : 125767 us
  link        sqlite3.so    :  18832 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1778 ms |
| RCC -O1   |       988 ms |
| RCC -O2   |      1076 ms |
| TCC       |       165 ms |
| GCC -O0   |      1900 ms |
| GCC -O2   |     14535 ms |
| Clang -O0 |      1344 ms |
| Clang -O2 |     11336 ms |
