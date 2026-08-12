# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          336 |          666 |       1002 |
| RCC -O1   |          118 |          811 |        929 |
| RCC -O2   |          121 |          803 |        924 |
| TCC       |           81 |          705 |        786 |
| GCC -O0   |          148 |          614 |        762 |
| GCC -O2   |          241 |          365 |        606 |
| Clang -O0 |          158 |          617 |        775 |
| Clang -O2 |          152 |          307 |        459 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    937 us
  parse       bench.c       :    151 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    141 us
  link        bench_rcc     :    238 us
  link        bench_rcc     :  88372 us

RCC -O1:
  preprocess  bench.c       :   1064 us
  parse       bench.c       :    190 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    182 us
  link        bench_o1      :    291 us
  link        bench_o1      :  91632 us

RCC -O2:
  preprocess  bench.c       :    704 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    119 us
  link        bench_o2      :    201 us
  link        bench_o2      :  67532 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 504679 us
  parse       sqlite3.c     : 287567 us
  typecheck   sqlite3.c     :  23862 us
  codegen     sqlite3.c     : 274090 us
  link        sqlite3.so    :  34691 us

RCC -O1:
  preprocess  sqlite3.c     : 408060 us
  parse       sqlite3.c     :  59265 us
  typecheck   sqlite3.c     :  36342 us
  opt         sqlite3.c     :  33426 us
  codegen     sqlite3.c     : 166485 us
  link        sqlite3.so    :  19325 us

RCC -O2:
  preprocess  sqlite3.c     : 449718 us
  parse       sqlite3.c     :  57002 us
  typecheck   sqlite3.c     :  19687 us
  opt         sqlite3.c     : 231273 us
  codegen     sqlite3.c     : 257805 us
  link        sqlite3.so    :  20637 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       974 ms |
| RCC -O1   |       716 ms |
| RCC -O2   |       892 ms |
| TCC       |       127 ms |
| GCC -O0   |      1378 ms |
| GCC -O2   |     17948 ms |
| Clang -O0 |      2683 ms |
| Clang -O2 |     22216 ms |
