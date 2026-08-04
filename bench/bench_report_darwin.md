# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           84 |          751 |        835 |
| RCC -O1   |          103 |          658 |        761 |
| RCC -O2   |           54 |          669 |        723 |
| TCC       |           54 |          751 |        805 |
| GCC -O0   |           78 |          548 |        626 |
| GCC -O2   |          182 |          325 |        507 |
| Clang -O0 |           88 |          593 |        681 |
| Clang -O2 |          167 |          351 |        518 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1371 us
  parse       bench.c       :    202 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    148 us
  link        bench_rcc     :     78 us
  link        bench_rcc     :  63229 us

RCC -O1:
  preprocess  bench.c       :    653 us
  parse       bench.c       :    189 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    150 us
  link        bench_o1      :    323 us
  link        bench_o1      :  52925 us

RCC -O2:
  preprocess  bench.c       :    710 us
  parse       bench.c       :    161 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    157 us
  link        bench_o2      :    305 us
  link        bench_o2      :  71020 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 383252 us
  parse       sqlite3.c     :  86824 us
  typecheck   sqlite3.c     :  19582 us
  codegen     sqlite3.c     : 170400 us
  link        sqlite3.so    :  18416 us

RCC -O1:
  preprocess  sqlite3.c     : 323919 us
  parse       sqlite3.c     :  69537 us
  typecheck   sqlite3.c     :  20197 us
  opt         sqlite3.c     :  29080 us
  codegen     sqlite3.c     : 164232 us
  link        sqlite3.so    :  16399 us

RCC -O2:
  preprocess  sqlite3.c     : 314385 us
  parse       sqlite3.c     :  53859 us
  typecheck   sqlite3.c     :  19129 us
  opt         sqlite3.c     : 204474 us
  codegen     sqlite3.c     : 144708 us
  link        sqlite3.so    :  18435 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1294 ms |
| RCC -O1   |       835 ms |
| RCC -O2   |       920 ms |
| TCC       |       203 ms |
| GCC -O0   |      1492 ms |
| GCC -O2   |     14638 ms |
| Clang -O0 |      1430 ms |
| Clang -O2 |     14247 ms |
