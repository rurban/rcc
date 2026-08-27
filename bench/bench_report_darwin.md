# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          147 |          945 |       1092 |
| RCC -O1   |          119 |          852 |        971 |
| RCC -O2   |           89 |          767 |        856 |
| TCC       |           57 |          638 |        695 |
| GCC -O0   |          104 |          600 |        704 |
| GCC -O2   |          178 |          337 |        515 |
| Clang -O0 |           66 |          511 |        577 |
| Clang -O2 |          123 |          316 |        439 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    880 us
  parse       bench.c       :    144 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    145 us
  link        bench_rcc     :    359 us
  link        bench_rcc     :  81896 us

RCC -O1:
  preprocess  bench.c       :   1079 us
  parse       bench.c       :    295 us
  typecheck   bench.c       :     11 us
  opt         bench.c       :     48 us
  codegen     bench.c       :    276 us
  link        bench_o1      :    347 us
  link        bench_o1      : 102662 us

RCC -O2:
  preprocess  bench.c       :   1352 us
  parse       bench.c       :    157 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    161 us
  link        bench_o2      :    256 us
  link        bench_o2      :  77425 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 465799 us
  parse       sqlite3.c     : 256159 us
  typecheck   sqlite3.c     :  36549 us
  codegen     sqlite3.c     : 208640 us
  link        sqlite3.so    :  20734 us

RCC -O1:
  preprocess  sqlite3.c     : 513342 us
  parse       sqlite3.c     : 108944 us
  typecheck   sqlite3.c     :  34905 us
  opt         sqlite3.c     : 318923 us
  codegen     sqlite3.c     : 296513 us
  link        sqlite3.so    :  33596 us

RCC -O2:
  preprocess  sqlite3.c     : 434510 us
  parse       sqlite3.c     :  96559 us
  typecheck   sqlite3.c     :  32124 us
  opt         sqlite3.c     : 363208 us
  codegen     sqlite3.c     : 241363 us
  link        sqlite3.so    :  21894 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1018 ms |
| RCC -O1   |       870 ms |
| RCC -O2   |       795 ms |
| TCC       |       132 ms |
| GCC -O0   |      1604 ms |
| GCC -O2   |     11508 ms |
| Clang -O0 |      1227 ms |
| Clang -O2 |     12703 ms |
