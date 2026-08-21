# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          139 |          669 |        808 |
| RCC -O1   |           58 |          683 |        741 |
| RCC -O2   |           61 |          655 |        716 |
| TCC       |           43 |          576 |        619 |
| GCC -O0   |          107 |          497 |        604 |
| GCC -O2   |          114 |          293 |        407 |
| Clang -O0 |           63 |          489 |        552 |
| Clang -O2 |           95 |          282 |        377 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    734 us
  parse       bench.c       :    169 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    156 us
  link        bench_rcc     :    225 us
  link        bench_rcc     :  48807 us

RCC -O1:
  preprocess  bench.c       :    645 us
  parse       bench.c       :    247 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    203 us
  link        bench_o1      :    102 us
  link        bench_o1      :  46626 us

RCC -O2:
  preprocess  bench.c       :    627 us
  parse       bench.c       :    174 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    161 us
  link        bench_o2      :    106 us
  link        bench_o2      :  45460 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 239926 us
  parse       sqlite3.c     :  62191 us
  typecheck   sqlite3.c     :  16964 us
  codegen     sqlite3.c     : 124139 us
  link        sqlite3.so    :  16289 us

RCC -O1:
  preprocess  sqlite3.c     : 241586 us
  parse       sqlite3.c     :  55502 us
  typecheck   sqlite3.c     :  14083 us
  opt         sqlite3.c     : 151215 us
  codegen     sqlite3.c     : 105748 us
  link        sqlite3.so    :  15969 us

RCC -O2:
  preprocess  sqlite3.c     : 223998 us
  parse       sqlite3.c     :  57673 us
  typecheck   sqlite3.c     :  18981 us
  opt         sqlite3.c     : 152838 us
  codegen     sqlite3.c     : 163654 us
  link        sqlite3.so    :  34392 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       949 ms |
| RCC -O1   |       851 ms |
| RCC -O2   |       836 ms |
| TCC       |       112 ms |
| GCC -O0   |      1100 ms |
| GCC -O2   |      9678 ms |
| Clang -O0 |       978 ms |
| Clang -O2 |     11503 ms |
