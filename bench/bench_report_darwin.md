# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          102 |          791 |        893 |
| RCC -O1   |           90 |          740 |        830 |
| RCC -O2   |           62 |          733 |        795 |
| TCC       |           82 |          672 |        754 |
| GCC -O0   |          104 |          576 |        680 |
| GCC -O2   |          224 |          305 |        529 |
| Clang -O0 |           77 |          590 |        667 |
| Clang -O2 |          117 |          311 |        428 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    711 us
  parse       bench.c       :    163 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    154 us
  link        bench_rcc     :    318 us
  link        bench_rcc     :  45096 us

RCC -O1:
  preprocess  bench.c       :    531 us
  parse       bench.c       :    114 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    111 us
  link        bench_o1      :    356 us
  link        bench_o1      :  44717 us

RCC -O2:
  preprocess  bench.c       :    696 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    144 us
  link        bench_o2      :    460 us
  link        bench_o2      : 108551 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 414338 us
  parse       sqlite3.c     : 245725 us
  typecheck   sqlite3.c     :  30447 us
  codegen     sqlite3.c     : 144223 us
  link        sqlite3.so    :  21209 us

RCC -O1:
  preprocess  sqlite3.c     : 336212 us
  parse       sqlite3.c     :  88057 us
  typecheck   sqlite3.c     :  14768 us
  opt         sqlite3.c     : 178655 us
  codegen     sqlite3.c     : 148098 us
  link        sqlite3.so    :  16130 us

RCC -O2:
  preprocess  sqlite3.c     : 371882 us
  parse       sqlite3.c     :  93088 us
  typecheck   sqlite3.c     :  16803 us
  opt         sqlite3.c     : 286880 us
  codegen     sqlite3.c     : 206700 us
  link        sqlite3.so    :  24396 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1286 ms |
| RCC -O1   |      1068 ms |
| RCC -O2   |       966 ms |
| TCC       |       139 ms |
| GCC -O0   |      1361 ms |
| GCC -O2   |     13225 ms |
| Clang -O0 |      1628 ms |
| Clang -O2 |     14678 ms |
