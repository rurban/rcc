# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           96 |          688 |        784 |
| RCC -O1   |           66 |          651 |        717 |
| RCC -O2   |           51 |          664 |        715 |
| TCC       |           51 |          540 |        591 |
| GCC -O0   |           58 |          524 |        582 |
| GCC -O2   |          151 |          283 |        434 |
| Clang -O0 |           75 |          459 |        534 |
| Clang -O2 |          140 |          298 |        438 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1219 us
  parse       bench.c       :    173 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    155 us
  link        bench_rcc     :    417 us
  link        bench_rcc     :  67412 us

RCC -O1:
  preprocess  bench.c       :    664 us
  parse       bench.c       :    136 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    143 us
  link        bench_o1      :    496 us
  link        bench_o1      :  50959 us

RCC -O2:
  preprocess  bench.c       :    603 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    138 us
  link        bench_o2      :    180 us
  link        bench_o2      :  59243 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 398397 us
  parse       sqlite3.c     : 172426 us
  typecheck   sqlite3.c     :  49617 us
  codegen     sqlite3.c     : 134486 us
  link        sqlite3.so    :  17580 us

RCC -O1:
  preprocess  sqlite3.c     : 294709 us
  parse       sqlite3.c     :  77060 us
  typecheck   sqlite3.c     :  25130 us
  opt         sqlite3.c     : 177698 us
  codegen     sqlite3.c     : 111681 us
  link        sqlite3.so    :  16021 us

RCC -O2:
  preprocess  sqlite3.c     : 235427 us
  parse       sqlite3.c     :  46895 us
  typecheck   sqlite3.c     :  12491 us
  opt         sqlite3.c     : 167552 us
  codegen     sqlite3.c     : 129985 us
  link        sqlite3.so    :  28539 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       842 ms |
| RCC -O1   |       972 ms |
| RCC -O2   |      1128 ms |
| TCC       |       113 ms |
| GCC -O0   |      1278 ms |
| GCC -O2   |     14714 ms |
| Clang -O0 |      1408 ms |
| Clang -O2 |     15045 ms |
