# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           54 |          625 |        679 |
| RCC -O1   |           53 |          626 |        679 |
| RCC -O2   |           54 |          630 |        684 |
| TCC       |           44 |          553 |        597 |
| GCC -O0   |           66 |          467 |        533 |
| GCC -O2   |          101 |          281 |        382 |
| Clang -O0 |           56 |          467 |        523 |
| Clang -O2 |           87 |          281 |        368 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    784 us
  parse       bench.c       :    168 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    118 us
  link        bench_rcc     :     90 us
  link        bench_rcc     :  55843 us

RCC -O1:
  preprocess  bench.c       :   1148 us
  parse       bench.c       :    242 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     55 us
  codegen     bench.c       :    176 us
  link        bench_o1      :    170 us
  link        bench_o1      :  58237 us

RCC -O2:
  preprocess  bench.c       :    828 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    143 us
  link        bench_o2      :    142 us
  link        bench_o2      :  60310 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 267710 us
  parse       sqlite3.c     :  58769 us
  typecheck   sqlite3.c     :  16419 us
  codegen     sqlite3.c     :  93097 us
  link        sqlite3.so    :  14713 us

RCC -O1:
  preprocess  sqlite3.c     : 222804 us
  parse       sqlite3.c     :  46045 us
  typecheck   sqlite3.c     :  12736 us
  opt         sqlite3.c     :  19837 us
  codegen     sqlite3.c     :  94395 us
  link        sqlite3.so    :  15629 us

RCC -O2:
  preprocess  sqlite3.c     : 208655 us
  parse       sqlite3.c     :  45455 us
  typecheck   sqlite3.c     :  12911 us
  opt         sqlite3.c     : 129249 us
  codegen     sqlite3.c     :  92889 us
  link        sqlite3.so    :  15501 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       573 ms |
| RCC -O1   |       601 ms |
| RCC -O2   |       722 ms |
| TCC       |        93 ms |
| GCC -O0   |      1068 ms |
| GCC -O2   |      9765 ms |
| Clang -O0 |      1128 ms |
| Clang -O2 |      9775 ms |
