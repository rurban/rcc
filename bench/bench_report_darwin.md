# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           56 |          634 |        690 |
| RCC -O1   |           84 |          663 |        747 |
| RCC -O2   |           68 |          633 |        701 |
| TCC       |           49 |          550 |        599 |
| GCC -O0   |           86 |          457 |        543 |
| GCC -O2   |          109 |          264 |        373 |
| Clang -O0 |           80 |          447 |        527 |
| Clang -O2 |           95 |          267 |        362 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1036 us
  parse       bench.c       :    213 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    224 us
  link        bench_rcc     :    350 us
  link        bench_rcc     :  76320 us

RCC -O1:
  preprocess  bench.c       :    753 us
  parse       bench.c       :    182 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     29 us
  codegen     bench.c       :    148 us
  link        bench_o1      :    181 us
  link        bench_o1      :  69930 us

RCC -O2:
  preprocess  bench.c       :    709 us
  parse       bench.c       :    170 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    160 us
  link        bench_o2      :    212 us
  link        bench_o2      :  63946 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 255168 us
  parse       sqlite3.c     :  59021 us
  typecheck   sqlite3.c     :  13978 us
  codegen     sqlite3.c     : 116441 us
  link        sqlite3.so    :  18307 us

RCC -O1:
  preprocess  sqlite3.c     : 229781 us
  parse       sqlite3.c     :  58719 us
  typecheck   sqlite3.c     :  13317 us
  opt         sqlite3.c     : 153481 us
  codegen     sqlite3.c     : 113679 us
  link        sqlite3.so    :  16406 us

RCC -O2:
  preprocess  sqlite3.c     : 202730 us
  parse       sqlite3.c     :  47745 us
  typecheck   sqlite3.c     :  10750 us
  opt         sqlite3.c     : 139455 us
  codegen     sqlite3.c     :  96486 us
  link        sqlite3.so    :  14978 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       755 ms |
| RCC -O1   |       775 ms |
| RCC -O2   |       793 ms |
| TCC       |       104 ms |
| GCC -O0   |      1796 ms |
| GCC -O2   |     14128 ms |
| Clang -O0 |      2006 ms |
| Clang -O2 |     17637 ms |
