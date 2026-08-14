# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           48 |          624 |        672 |
| RCC -O1   |           55 |          606 |        661 |
| RCC -O2   |           58 |          594 |        652 |
| TCC       |           44 |          548 |        592 |
| GCC -O0   |           69 |          457 |        526 |
| GCC -O2   |          103 |          267 |        370 |
| Clang -O0 |           63 |          471 |        534 |
| Clang -O2 |          178 |          309 |        487 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    687 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    135 us
  link        bench_rcc     :     96 us
  link        bench_rcc     :  48017 us

RCC -O1:
  preprocess  bench.c       :    589 us
  parse       bench.c       :    130 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    129 us
  link        bench_o1      :    166 us
  link        bench_o1      :  44904 us

RCC -O2:
  preprocess  bench.c       :    581 us
  parse       bench.c       :    130 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    125 us
  link        bench_o2      :    199 us
  link        bench_o2      :  52084 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 256825 us
  parse       sqlite3.c     :  45523 us
  typecheck   sqlite3.c     :  11935 us
  codegen     sqlite3.c     :  90440 us
  link        sqlite3.so    :  14813 us

RCC -O1:
  preprocess  sqlite3.c     : 216411 us
  parse       sqlite3.c     :  44525 us
  typecheck   sqlite3.c     :  11665 us
  opt         sqlite3.c     : 118651 us
  codegen     sqlite3.c     :  85953 us
  link        sqlite3.so    :  14907 us

RCC -O2:
  preprocess  sqlite3.c     : 217502 us
  parse       sqlite3.c     :  44121 us
  typecheck   sqlite3.c     :  11654 us
  opt         sqlite3.c     : 119168 us
  codegen     sqlite3.c     :  84973 us
  link        sqlite3.so    :  14716 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       679 ms |
| RCC -O1   |       678 ms |
| RCC -O2   |       675 ms |
| TCC       |       104 ms |
| GCC -O0   |       978 ms |
| GCC -O2   |      9806 ms |
| Clang -O0 |      1081 ms |
| Clang -O2 |     10188 ms |
