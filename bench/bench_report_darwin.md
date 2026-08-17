# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           67 |          630 |        697 |
| RCC -O1   |           56 |          646 |        702 |
| RCC -O2   |           62 |          640 |        702 |
| TCC       |           44 |          557 |        601 |
| GCC -O0   |           70 |          471 |        541 |
| GCC -O2   |          113 |          290 |        403 |
| Clang -O0 |           61 |          460 |        521 |
| Clang -O2 |           90 |          275 |        365 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    793 us
  parse       bench.c       :    181 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    144 us
  link        bench_rcc     :    142 us
  link        bench_rcc     :  55026 us

RCC -O1:
  preprocess  bench.c       :    756 us
  parse       bench.c       :    147 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    119 us
  link        bench_o1      :     82 us
  link        bench_o1      :  53215 us

RCC -O2:
  preprocess  bench.c       :    742 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    127 us
  link        bench_o2      :    122 us
  link        bench_o2      :  53211 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 281959 us
  parse       sqlite3.c     :  80321 us
  typecheck   sqlite3.c     :  21451 us
  codegen     sqlite3.c     : 151350 us
  link        sqlite3.so    :  17892 us

RCC -O1:
  preprocess  sqlite3.c     : 255451 us
  parse       sqlite3.c     :  55317 us
  typecheck   sqlite3.c     :  15241 us
  opt         sqlite3.c     : 152392 us
  codegen     sqlite3.c     : 120350 us
  link        sqlite3.so    :  15252 us

RCC -O2:
  preprocess  sqlite3.c     : 253194 us
  parse       sqlite3.c     :  60837 us
  typecheck   sqlite3.c     :  22080 us
  opt         sqlite3.c     : 174112 us
  codegen     sqlite3.c     :  99387 us
  link        sqlite3.so    :  15994 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       712 ms |
| RCC -O1   |       774 ms |
| RCC -O2   |       762 ms |
| TCC       |       139 ms |
| GCC -O0   |      1054 ms |
| GCC -O2   |     14563 ms |
| Clang -O0 |      1899 ms |
| Clang -O2 |     13185 ms |
