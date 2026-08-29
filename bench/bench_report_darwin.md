# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           66 |          648 |        714 |
| RCC -O1   |           67 |          643 |        710 |
| RCC -O2   |           62 |          649 |        711 |
| TCC       |           94 |          659 |        753 |
| GCC -O0   |          104 |          555 |        659 |
| GCC -O2   |          143 |          311 |        454 |
| Clang -O0 |           62 |          451 |        513 |
| Clang -O2 |           83 |          273 |        356 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    938 us
  parse       bench.c       :    180 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    139 us
  link        bench_rcc     :    297 us
  link        bench_rcc     :  76716 us

RCC -O1:
  preprocess  bench.c       :    636 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    139 us
  link        bench_o1      :    194 us
  link        bench_o1      :  64052 us

RCC -O2:
  preprocess  bench.c       :    652 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    132 us
  link        bench_o2      :    305 us
  link        bench_o2      :  72588 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 276631 us
  parse       sqlite3.c     :  63515 us
  typecheck   sqlite3.c     :  13481 us
  codegen     sqlite3.c     : 107058 us
  link        sqlite3.so    :  14852 us

RCC -O1:
  preprocess  sqlite3.c     : 201128 us
  parse       sqlite3.c     :  51378 us
  typecheck   sqlite3.c     :  10676 us
  opt         sqlite3.c     : 138515 us
  codegen     sqlite3.c     :  97962 us
  link        sqlite3.so    :  15584 us

RCC -O2:
  preprocess  sqlite3.c     : 191817 us
  parse       sqlite3.c     :  62271 us
  typecheck   sqlite3.c     :  14983 us
  opt         sqlite3.c     : 267811 us
  codegen     sqlite3.c     : 101738 us
  link        sqlite3.so    :  16750 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       712 ms |
| RCC -O1   |       719 ms |
| RCC -O2   |       681 ms |
| TCC       |        93 ms |
| GCC -O0   |       972 ms |
| GCC -O2   |     11398 ms |
| Clang -O0 |      1199 ms |
| Clang -O2 |     11094 ms |
