# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          102 |          629 |        731 |
| RCC -O1   |           71 |          703 |        774 |
| RCC -O2   |           70 |          686 |        756 |
| TCC       |           88 |          543 |        631 |
| GCC -O0   |          103 |          512 |        615 |
| GCC -O2   |          115 |          294 |        409 |
| Clang -O0 |           84 |          499 |        583 |
| Clang -O2 |          123 |          303 |        426 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    791 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    143 us
  link        bench_rcc     :    149 us
  link        bench_rcc     :  60347 us

RCC -O1:
  preprocess  bench.c       :    678 us
  parse       bench.c       :    136 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    177 us
  link        bench_o1      :    253 us
  link        bench_o1      :  79294 us

RCC -O2:
  preprocess  bench.c       :   1518 us
  parse       bench.c       :    302 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     50 us
  codegen     bench.c       :    274 us
  link        bench_o2      :    255 us
  link        bench_o2      :  78363 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 291154 us
  parse       sqlite3.c     :  83914 us
  typecheck   sqlite3.c     :  24918 us
  codegen     sqlite3.c     : 130714 us
  link        sqlite3.so    :  16678 us

RCC -O1:
  preprocess  sqlite3.c     : 328735 us
  parse       sqlite3.c     :  55189 us
  typecheck   sqlite3.c     :  18247 us
  opt         sqlite3.c     : 209214 us
  codegen     sqlite3.c     : 124050 us
  link        sqlite3.so    :  20879 us

RCC -O2:
  preprocess  sqlite3.c     : 242816 us
  parse       sqlite3.c     :  56693 us
  typecheck   sqlite3.c     :  13078 us
  opt         sqlite3.c     : 186792 us
  codegen     sqlite3.c     : 132716 us
  link        sqlite3.so    :  23867 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       702 ms |
| RCC -O1   |       845 ms |
| RCC -O2   |      1851 ms |
| TCC       |       153 ms |
| GCC -O0   |      1536 ms |
| GCC -O2   |     12676 ms |
| Clang -O0 |      1283 ms |
| Clang -O2 |     11530 ms |
