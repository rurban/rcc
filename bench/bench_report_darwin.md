# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           65 |          629 |        694 |
| RCC -O1   |           51 |          638 |        689 |
| RCC -O2   |           62 |          638 |        700 |
| TCC       |           55 |          549 |        604 |
| GCC -O0   |           60 |          458 |        518 |
| GCC -O2   |          104 |          276 |        380 |
| Clang -O0 |           60 |          478 |        538 |
| Clang -O2 |          107 |          286 |        393 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1251 us
  parse       bench.c       :    171 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    169 us
  link        bench_rcc     :    268 us
  link        bench_rcc     :  61752 us

RCC -O1:
  preprocess  bench.c       :    629 us
  parse       bench.c       :    171 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     29 us
  codegen     bench.c       :    136 us
  link        bench_o1      :    131 us
  link        bench_o1      :  54303 us

RCC -O2:
  preprocess  bench.c       :    807 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    136 us
  link        bench_o2      :    414 us
  link        bench_o2      :  57941 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 232015 us
  parse       sqlite3.c     :  56162 us
  typecheck   sqlite3.c     :  21439 us
  codegen     sqlite3.c     : 114944 us
  link        sqlite3.so    :  13754 us

RCC -O1:
  preprocess  sqlite3.c     : 209066 us
  parse       sqlite3.c     :  45121 us
  typecheck   sqlite3.c     :  12542 us
  opt         sqlite3.c     : 138484 us
  codegen     sqlite3.c     : 114614 us
  link        sqlite3.so    :  16129 us

RCC -O2:
  preprocess  sqlite3.c     : 209414 us
  parse       sqlite3.c     :  52700 us
  typecheck   sqlite3.c     :  13647 us
  opt         sqlite3.c     : 153273 us
  codegen     sqlite3.c     : 110017 us
  link        sqlite3.so    :  15413 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       641 ms |
| RCC -O1   |       768 ms |
| RCC -O2   |       768 ms |
| TCC       |       101 ms |
| GCC -O0   |      1063 ms |
| GCC -O2   |     10390 ms |
| Clang -O0 |      1205 ms |
| Clang -O2 |     10616 ms |
