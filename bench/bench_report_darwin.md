# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           83 |          683 |        766 |
| RCC -O1   |           74 |          688 |        762 |
| RCC -O2   |           68 |          680 |        748 |
| TCC       |           68 |          607 |        675 |
| GCC -O0   |           80 |          506 |        586 |
| GCC -O2   |          117 |          299 |        416 |
| Clang -O0 |           69 |          511 |        580 |
| Clang -O2 |          122 |          296 |        418 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1636 us
  parse       bench.c       :    175 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    160 us
  link        bench_rcc     :    177 us
  link        bench_rcc     : 100472 us

RCC -O1:
  preprocess  bench.c       :    806 us
  parse       bench.c       :    155 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    277 us
  link        bench_o1      :    195 us
  link        bench_o1      :  86674 us

RCC -O2:
  preprocess  bench.c       :    741 us
  parse       bench.c       :    173 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    359 us
  link        bench_o2      :    131 us
  link        bench_o2      :  90458 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 344989 us
  parse       sqlite3.c     :  87540 us
  typecheck   sqlite3.c     :  17421 us
  codegen     sqlite3.c     : 124518 us
  link        sqlite3.so    :  16740 us

RCC -O1:
  preprocess  sqlite3.c     : 239498 us
  parse       sqlite3.c     :  55485 us
  typecheck   sqlite3.c     :  14275 us
  opt         sqlite3.c     : 144744 us
  codegen     sqlite3.c     : 109423 us
  link        sqlite3.so    :  17533 us

RCC -O2:
  preprocess  sqlite3.c     : 224604 us
  parse       sqlite3.c     :  55048 us
  typecheck   sqlite3.c     :  14223 us
  opt         sqlite3.c     : 153021 us
  codegen     sqlite3.c     : 136808 us
  link        sqlite3.so    :  18865 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       782 ms |
| RCC -O1   |       823 ms |
| RCC -O2   |       821 ms |
| TCC       |       110 ms |
| GCC -O0   |      1151 ms |
| GCC -O2   |     11574 ms |
| Clang -O0 |      1166 ms |
| Clang -O2 |     10899 ms |
