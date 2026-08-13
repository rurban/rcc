# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          114 |          838 |        952 |
| RCC -O1   |           93 |          821 |        914 |
| RCC -O2   |           81 |          797 |        878 |
| TCC       |           89 |          680 |        769 |
| GCC -O0   |          104 |          569 |        673 |
| GCC -O2   |          185 |          330 |        515 |
| Clang -O0 |           82 |          581 |        663 |
| Clang -O2 |          220 |          365 |        585 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    992 us
  parse       bench.c       :    131 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    136 us
  link        bench_rcc     :    209 us
  link        bench_rcc     :  70488 us

RCC -O1:
  preprocess  bench.c       :    718 us
  parse       bench.c       :    322 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    169 us
  link        bench_o1      :    363 us
  link        bench_o1      :  87653 us

RCC -O2:
  preprocess  bench.c       :    817 us
  parse       bench.c       :    225 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    149 us
  link        bench_o2      :    383 us
  link        bench_o2      :  76692 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 348393 us
  parse       sqlite3.c     : 262889 us
  typecheck   sqlite3.c     :  25929 us
  codegen     sqlite3.c     : 197562 us
  link        sqlite3.so    :  19729 us

RCC -O1:
  preprocess  sqlite3.c     : 392733 us
  parse       sqlite3.c     :  81235 us
  typecheck   sqlite3.c     :  23656 us
  opt         sqlite3.c     : 246031 us
  codegen     sqlite3.c     : 208101 us
  link        sqlite3.so    :  28210 us

RCC -O2:
  preprocess  sqlite3.c     : 349377 us
  parse       sqlite3.c     :  63207 us
  typecheck   sqlite3.c     :  20322 us
  opt         sqlite3.c     : 227862 us
  codegen     sqlite3.c     : 238131 us
  link        sqlite3.so    :  84866 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1545 ms |
| RCC -O1   |      1872 ms |
| RCC -O2   |      1679 ms |
| TCC       |       190 ms |
| GCC -O0   |      2081 ms |
| GCC -O2   |     17623 ms |
| Clang -O0 |      1937 ms |
| Clang -O2 |     15015 ms |
