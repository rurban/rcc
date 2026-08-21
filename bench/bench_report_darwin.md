# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          144 |          801 |        945 |
| RCC -O1   |           96 |          807 |        903 |
| RCC -O2   |           92 |          732 |        824 |
| TCC       |           82 |          746 |        828 |
| GCC -O0   |          137 |          593 |        730 |
| GCC -O2   |          141 |          298 |        439 |
| Clang -O0 |           64 |          490 |        554 |
| Clang -O2 |          111 |          289 |        400 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1507 us
  parse       bench.c       :    201 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    199 us
  link        bench_rcc     :     92 us
  link        bench_rcc     :  50868 us

RCC -O1:
  preprocess  bench.c       :    857 us
  parse       bench.c       :    195 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    204 us
  link        bench_o1      :    315 us
  link        bench_o1      :  51127 us

RCC -O2:
  preprocess  bench.c       :   1635 us
  parse       bench.c       :    491 us
  typecheck   bench.c       :     11 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    320 us
  link        bench_o2      :   1277 us
  link        bench_o2      :  75020 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 389051 us
  parse       sqlite3.c     : 255542 us
  typecheck   sqlite3.c     :  43820 us
  codegen     sqlite3.c     : 271321 us
  link        sqlite3.so    :  28921 us

RCC -O1:
  preprocess  sqlite3.c     : 274010 us
  parse       sqlite3.c     :  95924 us
  typecheck   sqlite3.c     :  26268 us
  opt         sqlite3.c     : 273895 us
  codegen     sqlite3.c     : 230282 us
  link        sqlite3.so    :  23332 us

RCC -O2:
  preprocess  sqlite3.c     : 342347 us
  parse       sqlite3.c     :  73076 us
  typecheck   sqlite3.c     :  19417 us
  opt         sqlite3.c     : 219417 us
  codegen     sqlite3.c     : 170769 us
  link        sqlite3.so    :  27056 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       855 ms |
| RCC -O1   |       843 ms |
| RCC -O2   |       982 ms |
| TCC       |       142 ms |
| GCC -O0   |      1672 ms |
| GCC -O2   |     13765 ms |
| Clang -O0 |      2234 ms |
| Clang -O2 |     16112 ms |
