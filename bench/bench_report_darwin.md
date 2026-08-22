# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           61 |          637 |        698 |
| RCC -O1   |           51 |          632 |        683 |
| RCC -O2   |           55 |          639 |        694 |
| TCC       |           43 |          545 |        588 |
| GCC -O0   |           61 |          451 |        512 |
| GCC -O2   |          100 |          272 |        372 |
| Clang -O0 |           57 |          446 |        503 |
| Clang -O2 |           85 |          275 |        360 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    956 us
  parse       bench.c       :    198 us
  typecheck   bench.c       :      7 us
  codegen     bench.c       :    187 us
  link        bench_rcc     :    582 us
  link        bench_rcc     :  71722 us

RCC -O1:
  preprocess  bench.c       :    918 us
  parse       bench.c       :    175 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    141 us
  link        bench_o1      :    381 us
  link        bench_o1      :  65113 us

RCC -O2:
  preprocess  bench.c       :    753 us
  parse       bench.c       :    174 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    157 us
  link        bench_o2      :    418 us
  link        bench_o2      :  62237 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 298279 us
  parse       sqlite3.c     :  54315 us
  typecheck   sqlite3.c     :  13990 us
  codegen     sqlite3.c     : 106607 us
  link        sqlite3.so    :  15285 us

RCC -O1:
  preprocess  sqlite3.c     : 198492 us
  parse       sqlite3.c     :  46283 us
  typecheck   sqlite3.c     :  13571 us
  opt         sqlite3.c     : 134851 us
  codegen     sqlite3.c     :  92504 us
  link        sqlite3.so    :  15738 us

RCC -O2:
  preprocess  sqlite3.c     : 207528 us
  parse       sqlite3.c     :  51354 us
  typecheck   sqlite3.c     :  13026 us
  opt         sqlite3.c     : 137215 us
  codegen     sqlite3.c     :  94324 us
  link        sqlite3.so    :  16309 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       543 ms |
| RCC -O1   |       678 ms |
| RCC -O2   |       783 ms |
| TCC       |       107 ms |
| GCC -O0   |      1035 ms |
| GCC -O2   |     10266 ms |
| Clang -O0 |      1115 ms |
| Clang -O2 |      9910 ms |
