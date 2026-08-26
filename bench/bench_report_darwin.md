# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          104 |          737 |        841 |
| RCC -O1   |           83 |          699 |        782 |
| RCC -O2   |           99 |          652 |        751 |
| TCC       |           59 |          577 |        636 |
| GCC -O0   |           83 |          459 |        542 |
| GCC -O2   |          112 |          269 |        381 |
| Clang -O0 |           60 |          446 |        506 |
| Clang -O2 |          110 |          295 |        405 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1929 us
  parse       bench.c       :    384 us
  typecheck   bench.c       :     10 us
  codegen     bench.c       :    360 us
  link        bench_rcc     :    179 us
  link        bench_rcc     :  82842 us

RCC -O1:
  preprocess  bench.c       :   1253 us
  parse       bench.c       :    252 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    184 us
  link        bench_o1      :    283 us
  link        bench_o1      :  77464 us

RCC -O2:
  preprocess  bench.c       :   1924 us
  parse       bench.c       :    180 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    148 us
  link        bench_o2      :    380 us
  link        bench_o2      :  95374 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 353187 us
  parse       sqlite3.c     : 148059 us
  typecheck   sqlite3.c     :  29412 us
  codegen     sqlite3.c     : 146275 us
  link        sqlite3.so    :  22969 us

RCC -O1:
  preprocess  sqlite3.c     : 381744 us
  parse       sqlite3.c     :  59024 us
  typecheck   sqlite3.c     :  21987 us
  opt         sqlite3.c     : 217977 us
  codegen     sqlite3.c     : 161584 us
  link        sqlite3.so    :  16820 us

RCC -O2:
  preprocess  sqlite3.c     : 263841 us
  parse       sqlite3.c     :  58386 us
  typecheck   sqlite3.c     :  15268 us
  opt         sqlite3.c     : 235958 us
  codegen     sqlite3.c     : 172108 us
  link        sqlite3.so    :  23893 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       650 ms |
| RCC -O1   |      1018 ms |
| RCC -O2   |       853 ms |
| TCC       |       109 ms |
| GCC -O0   |      1083 ms |
| GCC -O2   |     11391 ms |
| Clang -O0 |      1453 ms |
| Clang -O2 |     10591 ms |
