# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          173 |          921 |       1094 |
| RCC -O1   |          123 |          839 |        962 |
| RCC -O2   |          186 |          875 |       1061 |
| TCC       |          118 |          801 |        919 |
| GCC -O0   |          248 |          722 |        970 |
| GCC -O2   |          306 |          379 |        685 |
| Clang -O0 |          133 |          631 |        764 |
| Clang -O2 |          187 |          354 |        541 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          468 |         7217 |       7685 |
| RCC -O1   |          309 |         7344 |       7653 |
| RCC -O2   |          204 |         6266 |       6470 |
| TCC       |          346 |         5825 |       6171 |
| GCC -O0   |          676 |         3326 |       4002 |
| GCC -O2   |          935 |         1893 |       2828 |
| Clang -O0 |          527 |         3580 |       4107 |
| Clang -O2 |         1043 |         1891 |       2934 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1243 us
  parse       bench.c       :    362 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    216 us
  link        bench_rcc     :    324 us
  link        bench_rcc     :  83772 us

RCC -O1:
  preprocess  bench.c       :    933 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    140 us
  link        bench_o1      :    367 us
  link        bench_o1      :  89168 us

RCC -O2:
  preprocess  bench.c       :    759 us
  parse       bench.c       :    170 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    161 us
  link        bench_o2      :    374 us
  link        bench_o2      :  85017 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 361594 us
  parse       sqlite3.c     : 248595 us
  typecheck   sqlite3.c     :  41250 us
  codegen     sqlite3.c     : 231984 us
  link        sqlite3.so    :  34327 us

RCC -O1:
  preprocess  sqlite3.c     : 442566 us
  parse       sqlite3.c     :  72296 us
  typecheck   sqlite3.c     :  13154 us
  opt         sqlite3.c     : 290452 us
  codegen     sqlite3.c     : 168236 us
  link        sqlite3.so    :  23431 us

RCC -O2:
  preprocess  sqlite3.c     : 370318 us
  parse       sqlite3.c     :  96370 us
  typecheck   sqlite3.c     :  16936 us
  opt         sqlite3.c     : 458835 us
  codegen     sqlite3.c     : 384492 us
  link        sqlite3.so    :  35019 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1080 ms |
| RCC -O1   |       756 ms |
| RCC -O2   |       753 ms |
| TCC       |       109 ms |
| GCC -O0   |      1174 ms |
| GCC -O2   |     10145 ms |
| Clang -O0 |      1033 ms |
| Clang -O2 |      9784 ms |
