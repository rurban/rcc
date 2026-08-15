# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           49 |          601 |        650 |
| RCC -O1   |           68 |          594 |        662 |
| RCC -O2   |           52 |          594 |        646 |
| TCC       |           42 |          512 |        554 |
| GCC -O0   |           60 |          434 |        494 |
| GCC -O2   |           89 |          263 |        352 |
| Clang -O0 |           51 |          433 |        484 |
| Clang -O2 |           80 |          263 |        343 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    640 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    120 us
  link        bench_rcc     :    212 us
  link        bench_rcc     :  43368 us

RCC -O1:
  preprocess  bench.c       :    561 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    109 us
  link        bench_o1      :    300 us
  link        bench_o1      :  42808 us

RCC -O2:
  preprocess  bench.c       :    575 us
  parse       bench.c       :    122 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    126 us
  link        bench_o2      :    325 us
  link        bench_o2      :  42972 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 223643 us
  parse       sqlite3.c     :  50150 us
  typecheck   sqlite3.c     :  11913 us
  codegen     sqlite3.c     :  90814 us
  link        sqlite3.so    :  18181 us

RCC -O1:
  preprocess  sqlite3.c     : 199797 us
  parse       sqlite3.c     :  46092 us
  typecheck   sqlite3.c     :  11760 us
  opt         sqlite3.c     : 120301 us
  codegen     sqlite3.c     :  90809 us
  link        sqlite3.so    :  14892 us

RCC -O2:
  preprocess  sqlite3.c     : 193611 us
  parse       sqlite3.c     :  41966 us
  typecheck   sqlite3.c     :  11572 us
  opt         sqlite3.c     : 123079 us
  codegen     sqlite3.c     :  95093 us
  link        sqlite3.so    :  14643 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       654 ms |
| RCC -O1   |       689 ms |
| RCC -O2   |       648 ms |
| TCC       |        87 ms |
| GCC -O0   |       913 ms |
| GCC -O2   |      8668 ms |
| Clang -O0 |       979 ms |
| Clang -O2 |      9859 ms |
