# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          203 |         1343 |       1546 |
| RCC -O1   |          181 |          914 |       1095 |
| RCC -O2   |          105 |          885 |        990 |
| TCC       |           98 |          682 |        780 |
| GCC -O0   |          168 |          558 |        726 |
| GCC -O2   |          187 |          315 |        502 |
| Clang -O0 |           84 |          553 |        637 |
| Clang -O2 |          200 |          309 |        509 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1310 us
  parse       bench.c       :    515 us
  typecheck   bench.c       :     15 us
  codegen     bench.c       :    289 us
  link        bench_rcc     :    649 us
  link        bench_rcc     :  87240 us

RCC -O1:
  preprocess  bench.c       :    872 us
  parse       bench.c       :    196 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    191 us
  link        bench_o1      :    184 us
  link        bench_o1      :  82628 us

RCC -O2:
  preprocess  bench.c       :    700 us
  parse       bench.c       :    238 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    150 us
  link        bench_o2      :    190 us
  link        bench_o2      :  88582 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 516842 us
  parse       sqlite3.c     : 203017 us
  typecheck   sqlite3.c     :  50337 us
  codegen     sqlite3.c     : 206050 us
  link        sqlite3.so    :  17210 us

RCC -O1:
  preprocess  sqlite3.c     : 473345 us
  parse       sqlite3.c     :  88021 us
  typecheck   sqlite3.c     :  27052 us
  opt         sqlite3.c     : 321058 us
  codegen     sqlite3.c     : 241520 us
  link        sqlite3.so    :  19913 us

RCC -O2:
  preprocess  sqlite3.c     : 516610 us
  parse       sqlite3.c     : 107026 us
  typecheck   sqlite3.c     :  28070 us
  opt         sqlite3.c     : 375282 us
  codegen     sqlite3.c     : 336882 us
  link        sqlite3.so    :  27817 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       838 ms |
| RCC -O1   |       945 ms |
| RCC -O2   |       916 ms |
| TCC       |       128 ms |
| GCC -O0   |      1392 ms |
| GCC -O2   |     16460 ms |
| Clang -O0 |      1644 ms |
| Clang -O2 |     12348 ms |
