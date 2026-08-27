# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          100 |          773 |        873 |
| RCC -O1   |          117 |          698 |        815 |
| RCC -O2   |           96 |          695 |        791 |
| TCC       |           64 |          578 |        642 |
| GCC -O0   |          108 |          503 |        611 |
| GCC -O2   |          177 |          288 |        465 |
| Clang -O0 |           66 |          474 |        540 |
| Clang -O2 |           95 |          302 |        397 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1009 us
  parse       bench.c       :    201 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    225 us
  link        bench_rcc     :    280 us
  link        bench_rcc     :  79674 us

RCC -O1:
  preprocess  bench.c       :    830 us
  parse       bench.c       :    198 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    142 us
  link        bench_o1      :    676 us
  link        bench_o1      :  63707 us

RCC -O2:
  preprocess  bench.c       :    853 us
  parse       bench.c       :    157 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    134 us
  link        bench_o2      :    157 us
  link        bench_o2      :  83373 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 379047 us
  parse       sqlite3.c     : 223130 us
  typecheck   sqlite3.c     :  25799 us
  codegen     sqlite3.c     : 192421 us
  link        sqlite3.so    :  20681 us

RCC -O1:
  preprocess  sqlite3.c     : 340815 us
  parse       sqlite3.c     :  58880 us
  typecheck   sqlite3.c     :  17921 us
  opt         sqlite3.c     : 225628 us
  codegen     sqlite3.c     : 212188 us
  link        sqlite3.so    :  24367 us

RCC -O2:
  preprocess  sqlite3.c     : 500072 us
  parse       sqlite3.c     : 122474 us
  typecheck   sqlite3.c     :  27849 us
  opt         sqlite3.c     : 334648 us
  codegen     sqlite3.c     : 192498 us
  link        sqlite3.so    :  25977 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       727 ms |
| RCC -O1   |       760 ms |
| RCC -O2   |       760 ms |
| TCC       |       453 ms |
| GCC -O0   |      1335 ms |
| GCC -O2   |     15980 ms |
| Clang -O0 |      1766 ms |
| Clang -O2 |     13558 ms |
