# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           81 |          728 |        809 |
| RCC -O1   |          124 |          897 |       1021 |
| RCC -O2   |           95 |          662 |        757 |
| TCC       |           47 |          585 |        632 |
| GCC -O0   |          107 |          474 |        581 |
| GCC -O2   |          112 |          289 |        401 |
| Clang -O0 |           83 |          481 |        564 |
| Clang -O2 |          113 |          294 |        407 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1442 us
  parse       bench.c       :    286 us
  typecheck   bench.c       :      9 us
  codegen     bench.c       :    298 us
  link        bench_rcc     :    534 us
  link        bench_rcc     :  94373 us

RCC -O1:
  preprocess  bench.c       :    606 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    132 us
  link        bench_o1      :    210 us
  link        bench_o1      : 126498 us

RCC -O2:
  preprocess  bench.c       :    691 us
  parse       bench.c       :    130 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    129 us
  link        bench_o2      :    154 us
  link        bench_o2      :  81139 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 365019 us
  parse       sqlite3.c     : 115890 us
  typecheck   sqlite3.c     :  30563 us
  codegen     sqlite3.c     : 244331 us
  link        sqlite3.so    :  21575 us

RCC -O1:
  preprocess  sqlite3.c     : 381624 us
  parse       sqlite3.c     :  79981 us
  typecheck   sqlite3.c     :  27658 us
  opt         sqlite3.c     : 210230 us
  codegen     sqlite3.c     : 153361 us
  link        sqlite3.so    :  19782 us

RCC -O2:
  preprocess  sqlite3.c     : 265342 us
  parse       sqlite3.c     :  63513 us
  typecheck   sqlite3.c     :  16235 us
  opt         sqlite3.c     : 191995 us
  codegen     sqlite3.c     : 148528 us
  link        sqlite3.so    :  20378 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1179 ms |
| RCC -O1   |       934 ms |
| RCC -O2   |       902 ms |
| TCC       |       127 ms |
| GCC -O0   |      1324 ms |
| GCC -O2   |     15357 ms |
| Clang -O0 |      1347 ms |
| Clang -O2 |     14039 ms |
