# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           78 |          681 |        759 |
| RCC -O1   |           82 |          670 |        752 |
| RCC -O2   |           64 |          716 |        780 |
| TCC       |           55 |          572 |        627 |
| GCC -O0   |           99 |          462 |        561 |
| GCC -O2   |          113 |          280 |        393 |
| Clang -O0 |           61 |          465 |        526 |
| Clang -O2 |          106 |          293 |        399 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1713 us
  parse       bench.c       :    342 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    306 us
  link        bench_rcc     :    524 us
  link        bench_rcc     :  90461 us

RCC -O1:
  preprocess  bench.c       :   1653 us
  parse       bench.c       :    329 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     55 us
  codegen     bench.c       :    349 us
  link        bench_o1      :    549 us
  link        bench_o1      :  94803 us

RCC -O2:
  preprocess  bench.c       :    816 us
  parse       bench.c       :    194 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    394 us
  link        bench_o2      :    461 us
  link        bench_o2      :  85116 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 410025 us
  parse       sqlite3.c     : 114714 us
  typecheck   sqlite3.c     :  16266 us
  codegen     sqlite3.c     : 115319 us
  link        sqlite3.so    :  17209 us

RCC -O1:
  preprocess  sqlite3.c     : 273811 us
  parse       sqlite3.c     :  68366 us
  typecheck   sqlite3.c     :  14161 us
  opt         sqlite3.c     : 154548 us
  codegen     sqlite3.c     : 135739 us
  link        sqlite3.so    :  16630 us

RCC -O2:
  preprocess  sqlite3.c     : 272554 us
  parse       sqlite3.c     :  73263 us
  typecheck   sqlite3.c     :  15181 us
  opt         sqlite3.c     : 190731 us
  codegen     sqlite3.c     : 127319 us
  link        sqlite3.so    :  18961 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       619 ms |
| RCC -O1   |       764 ms |
| RCC -O2   |       758 ms |
| TCC       |       114 ms |
| GCC -O0   |      1101 ms |
| GCC -O2   |     11443 ms |
| Clang -O0 |      1001 ms |
| Clang -O2 |      9574 ms |
