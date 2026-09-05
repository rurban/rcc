# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           72 |          617 |        689 |
| RCC -O1   |           75 |          619 |        694 |
| RCC -O2   |           75 |          625 |        700 |
| TCC       |           56 |          535 |        591 |
| GCC -O0   |           80 |          451 |        531 |
| GCC -O2   |          121 |          272 |        393 |
| Clang -O0 |           60 |          450 |        510 |
| Clang -O2 |           87 |          273 |        360 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          156 |         4843 |       4999 |
| RCC -O1   |          177 |         4796 |       4973 |
| RCC -O2   |          313 |         4209 |       4522 |
| TCC       |          133 |         4041 |       4174 |
| GCC -O0   |          640 |         3540 |       4180 |
| GCC -O2   |         1239 |         1827 |       3066 |
| Clang -O0 |          510 |         4185 |       4695 |
| Clang -O2 |         1513 |         2184 |       3697 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    742 us
  parse       bench.c       :    195 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    156 us
  link        bench_rcc     :     88 us
  link        bench_rcc     :  59314 us

RCC -O1:
  preprocess  bench.c       :    628 us
  parse       bench.c       :    206 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    161 us
  link        bench_o1      :    148 us
  link        bench_o1      :  55731 us

RCC -O2:
  preprocess  bench.c       :    650 us
  parse       bench.c       :    156 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    142 us
  link        bench_o2      :    161 us
  link        bench_o2      :  54112 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 226671 us
  parse       sqlite3.c     :  54871 us
  typecheck   sqlite3.c     :  13305 us
  codegen     sqlite3.c     : 120808 us
  link        sqlite3.so    :  16302 us

RCC -O1:
  preprocess  sqlite3.c     : 281541 us
  parse       sqlite3.c     :  62187 us
  typecheck   sqlite3.c     :  15475 us
  opt         sqlite3.c     : 175766 us
  codegen     sqlite3.c     : 137594 us
  link        sqlite3.so    :  16952 us

RCC -O2:
  preprocess  sqlite3.c     : 250867 us
  parse       sqlite3.c     :  52803 us
  typecheck   sqlite3.c     :  10850 us
  opt         sqlite3.c     : 149209 us
  codegen     sqlite3.c     : 126102 us
  link        sqlite3.so    :  14658 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       989 ms |
| RCC -O1   |      1042 ms |
| RCC -O2   |       826 ms |
| TCC       |       107 ms |
| GCC -O0   |      1249 ms |
| GCC -O2   |     10608 ms |
| Clang -O0 |      1404 ms |
| Clang -O2 |      9752 ms |
