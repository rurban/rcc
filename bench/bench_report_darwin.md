# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          140 |          897 |       1037 |
| RCC -O1   |          111 |          815 |        926 |
| RCC -O2   |           79 |          921 |       1000 |
| TCC       |           89 |          714 |        803 |
| GCC -O0   |          198 |          697 |        895 |
| GCC -O2   |          164 |          375 |        539 |
| Clang -O0 |           98 |          686 |        784 |
| Clang -O2 |          180 |          386 |        566 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    906 us
  parse       bench.c       :    171 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    185 us
  link        bench_rcc     :    633 us
  link        bench_rcc     :  68221 us

RCC -O1:
  preprocess  bench.c       :    614 us
  parse       bench.c       :    128 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    130 us
  link        bench_o1      :    123 us
  link        bench_o1      :  60934 us

RCC -O2:
  preprocess  bench.c       :    688 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    143 us
  link        bench_o2      :    394 us
  link        bench_o2      :  59237 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 457283 us
  parse       sqlite3.c     : 222016 us
  typecheck   sqlite3.c     :  40145 us
  codegen     sqlite3.c     : 214343 us
  link        sqlite3.so    :  20215 us

RCC -O1:
  preprocess  sqlite3.c     : 503511 us
  parse       sqlite3.c     : 129375 us
  typecheck   sqlite3.c     :  29881 us
  opt         sqlite3.c     : 306954 us
  codegen     sqlite3.c     : 213015 us
  link        sqlite3.so    :  28291 us

RCC -O2:
  preprocess  sqlite3.c     : 494927 us
  parse       sqlite3.c     :  92708 us
  typecheck   sqlite3.c     :  22535 us
  opt         sqlite3.c     : 291975 us
  codegen     sqlite3.c     : 182283 us
  link        sqlite3.so    :  22419 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1413 ms |
| RCC -O1   |      1280 ms |
| RCC -O2   |      1325 ms |
| TCC       |       217 ms |
| GCC -O0   |      2010 ms |
| GCC -O2   |     17572 ms |
| Clang -O0 |      2362 ms |
| Clang -O2 |     16236 ms |
