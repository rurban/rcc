# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          124 |          731 |        855 |
| RCC -O1   |           79 |          710 |        789 |
| RCC -O2   |          108 |          789 |        897 |
| TCC       |           64 |          579 |        643 |
| GCC -O0   |           96 |          482 |        578 |
| GCC -O2   |          144 |          301 |        445 |
| Clang -O0 |           74 |          520 |        594 |
| Clang -O2 |          194 |          323 |        517 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1008 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    161 us
  link        bench_rcc     :    358 us
  link        bench_rcc     :  57485 us

RCC -O1:
  preprocess  bench.c       :    631 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    132 us
  link        bench_o1      :    154 us
  link        bench_o1      :  54943 us

RCC -O2:
  preprocess  bench.c       :    710 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    115 us
  link        bench_o2      :    188 us
  link        bench_o2      :  54164 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 372963 us
  parse       sqlite3.c     : 220143 us
  typecheck   sqlite3.c     :  29210 us
  codegen     sqlite3.c     : 193802 us
  link        sqlite3.so    :  20135 us

RCC -O1:
  preprocess  sqlite3.c     : 285140 us
  parse       sqlite3.c     :  51289 us
  typecheck   sqlite3.c     :  13607 us
  opt         sqlite3.c     : 191459 us
  codegen     sqlite3.c     : 102388 us
  link        sqlite3.so    :  15291 us

RCC -O2:
  preprocess  sqlite3.c     : 293558 us
  parse       sqlite3.c     :  53906 us
  typecheck   sqlite3.c     :  14400 us
  opt         sqlite3.c     : 209997 us
  codegen     sqlite3.c     : 120607 us
  link        sqlite3.so    :  17655 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       937 ms |
| RCC -O1   |       871 ms |
| RCC -O2   |       778 ms |
| TCC       |       116 ms |
| GCC -O0   |      1313 ms |
| GCC -O2   |     15522 ms |
| Clang -O0 |      1773 ms |
| Clang -O2 |     15818 ms |
