# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           58 |          603 |        661 |
| RCC -O1   |           50 |          595 |        645 |
| RCC -O2   |           52 |          606 |        658 |
| TCC       |           39 |          513 |        552 |
| GCC -O0   |           67 |          461 |        528 |
| GCC -O2   |          138 |          273 |        411 |
| Clang -O0 |           55 |          441 |        496 |
| Clang -O2 |          106 |          263 |        369 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    742 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    117 us
  link        bench_rcc     :    109 us
  link        bench_rcc     :  54405 us

RCC -O1:
  preprocess  bench.c       :    621 us
  parse       bench.c       :    136 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    128 us
  link        bench_o1      :    166 us
  link        bench_o1      :  57378 us

RCC -O2:
  preprocess  bench.c       :    700 us
  parse       bench.c       :    136 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    139 us
  link        bench_o2      :    122 us
  link        bench_o2      :  55630 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 247962 us
  parse       sqlite3.c     :  60310 us
  typecheck   sqlite3.c     :  19589 us
  codegen     sqlite3.c     : 111368 us
  link        sqlite3.so    :  15545 us

RCC -O1:
  preprocess  sqlite3.c     : 218964 us
  parse       sqlite3.c     :  57151 us
  typecheck   sqlite3.c     :  15161 us
  opt         sqlite3.c     : 148723 us
  codegen     sqlite3.c     :  93403 us
  link        sqlite3.so    :  14245 us

RCC -O2:
  preprocess  sqlite3.c     : 208329 us
  parse       sqlite3.c     :  70163 us
  typecheck   sqlite3.c     :  13527 us
  opt         sqlite3.c     : 143113 us
  codegen     sqlite3.c     :  86362 us
  link        sqlite3.so    :  13627 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       743 ms |
| RCC -O1   |       699 ms |
| RCC -O2   |       772 ms |
| TCC       |       237 ms |
| GCC -O0   |      1058 ms |
| GCC -O2   |     14825 ms |
| Clang -O0 |      1215 ms |
| Clang -O2 |     13188 ms |
