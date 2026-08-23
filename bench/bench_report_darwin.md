# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          101 |          839 |        940 |
| RCC -O1   |           89 |          850 |        939 |
| RCC -O2   |           76 |          834 |        910 |
| TCC       |           53 |          634 |        687 |
| GCC -O0   |           99 |          521 |        620 |
| GCC -O2   |          126 |          294 |        420 |
| Clang -O0 |           76 |          486 |        562 |
| Clang -O2 |          125 |          293 |        418 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    897 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    160 us
  link        bench_rcc     :     66 us
  link        bench_rcc     :  61817 us

RCC -O1:
  preprocess  bench.c       :    695 us
  parse       bench.c       :    160 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    127 us
  link        bench_o1      :    154 us
  link        bench_o1      :  52178 us

RCC -O2:
  preprocess  bench.c       :    649 us
  parse       bench.c       :    131 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    123 us
  link        bench_o2      :    174 us
  link        bench_o2      :  46192 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 256702 us
  parse       sqlite3.c     :  72331 us
  typecheck   sqlite3.c     :  23478 us
  codegen     sqlite3.c     : 163190 us
  link        sqlite3.so    :  23430 us

RCC -O1:
  preprocess  sqlite3.c     : 266517 us
  parse       sqlite3.c     :  59194 us
  typecheck   sqlite3.c     :  15567 us
  opt         sqlite3.c     : 233831 us
  codegen     sqlite3.c     : 125504 us
  link        sqlite3.so    :  18501 us

RCC -O2:
  preprocess  sqlite3.c     : 288802 us
  parse       sqlite3.c     :  69943 us
  typecheck   sqlite3.c     :  16398 us
  opt         sqlite3.c     : 186972 us
  codegen     sqlite3.c     : 169078 us
  link        sqlite3.so    :  34851 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1094 ms |
| RCC -O1   |       890 ms |
| RCC -O2   |      1183 ms |
| TCC       |       172 ms |
| GCC -O0   |      1199 ms |
| GCC -O2   |     12620 ms |
| Clang -O0 |      1128 ms |
| Clang -O2 |     13293 ms |
