# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           91 |          804 |        895 |
| RCC -O1   |           84 |          805 |        889 |
| RCC -O2   |           83 |          783 |        866 |
| TCC       |           64 |          704 |        768 |
| GCC -O0   |           91 |          586 |        677 |
| GCC -O2   |          153 |          336 |        489 |
| Clang -O0 |           84 |          613 |        697 |
| Clang -O2 |          126 |          335 |        461 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    693 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    166 us
  link        bench_rcc     :    409 us
  link        bench_rcc     :  65395 us

RCC -O1:
  preprocess  bench.c       :    621 us
  parse       bench.c       :    122 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    117 us
  link        bench_o1      :    721 us
  link        bench_o1      :  55724 us

RCC -O2:
  preprocess  bench.c       :    685 us
  parse       bench.c       :    125 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    120 us
  link        bench_o2      :    145 us
  link        bench_o2      :  59071 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 330877 us
  parse       sqlite3.c     : 118314 us
  typecheck   sqlite3.c     :  20836 us
  codegen     sqlite3.c     : 143662 us
  link        sqlite3.so    :  16776 us

RCC -O1:
  preprocess  sqlite3.c     : 296644 us
  parse       sqlite3.c     :  61319 us
  typecheck   sqlite3.c     :  15212 us
  opt         sqlite3.c     : 205492 us
  codegen     sqlite3.c     : 121723 us
  link        sqlite3.so    :  16475 us

RCC -O2:
  preprocess  sqlite3.c     : 329961 us
  parse       sqlite3.c     :  74570 us
  typecheck   sqlite3.c     :  22213 us
  opt         sqlite3.c     : 216695 us
  codegen     sqlite3.c     : 128905 us
  link        sqlite3.so    :  16417 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       793 ms |
| RCC -O1   |       849 ms |
| RCC -O2   |      1021 ms |
| TCC       |       157 ms |
| GCC -O0   |      1620 ms |
| GCC -O2   |     15242 ms |
| Clang -O0 |      1661 ms |
| Clang -O2 |     17204 ms |
