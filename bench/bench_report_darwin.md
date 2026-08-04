# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          157 |          818 |        975 |
| RCC -O1   |           96 |          753 |        849 |
| RCC -O2   |           95 |          715 |        810 |
| TCC       |           57 |          613 |        670 |
| GCC -O0   |           99 |          520 |        619 |
| GCC -O2   |          168 |          297 |        465 |
| Clang -O0 |           74 |          487 |        561 |
| Clang -O2 |           90 |          287 |        377 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1700 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    186 us
  link        bench_rcc     :    200 us
  link        bench_rcc     :  78399 us

RCC -O1:
  preprocess  bench.c       :    658 us
  parse       bench.c       :    131 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    135 us
  link        bench_o1      :   3208 us
  link        bench_o1      :  65932 us

RCC -O2:
  preprocess  bench.c       :    543 us
  parse       bench.c       :    150 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    132 us
  link        bench_o2      :    492 us
  link        bench_o2      :  64923 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 431493 us
  parse       sqlite3.c     : 175961 us
  typecheck   sqlite3.c     :  42181 us
  codegen     sqlite3.c     : 175802 us
  link        sqlite3.so    :  19942 us

RCC -O1:
  preprocess  sqlite3.c     : 376293 us
  parse       sqlite3.c     :  63122 us
  typecheck   sqlite3.c     :  25920 us
  opt         sqlite3.c     :  40674 us
  codegen     sqlite3.c     : 163986 us
  link        sqlite3.so    :  35657 us

RCC -O2:
  preprocess  sqlite3.c     : 363678 us
  parse       sqlite3.c     :  74722 us
  typecheck   sqlite3.c     :  22423 us
  opt         sqlite3.c     : 235724 us
  codegen     sqlite3.c     : 203092 us
  link        sqlite3.so    :  17852 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       978 ms |
| RCC -O1   |       906 ms |
| RCC -O2   |      1288 ms |
| TCC       |       162 ms |
| GCC -O0   |      1864 ms |
| GCC -O2   |     15404 ms |
| Clang -O0 |      1584 ms |
| Clang -O2 |     16654 ms |
