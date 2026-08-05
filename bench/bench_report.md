# Linux RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           16 |          688 |        704 |
| RCC -O1   |           16 |          690 |        706 |
| RCC -O2   |           17 |          696 |        713 |
| TCC       |            9 |          499 |        508 |
| SLIMCC    |           37 |          513 |        550 |
| XCC       |           11 |          367 |        378 |
| KEFIR     |          185 |          585 |        770 |
| KEFIR -O1 |          197 |          316 |        513 |
| SCC       |           37 |          550 |        587 |
| LACC      |           26 |          775 |        801 |
| CANTCC    |           29 |          422 |        451 |
| CCC       |           37 |          549 |        586 |
| GCC -O0   |           59 |          488 |        547 |
| GCC -O2   |          152 |          198 |        350 |
| Clang -O0 |           79 |          503 |        582 |
| Clang -O2 |          137 |          189 |        326 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   9094 us
  parse       bench.c       :    545 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    456 us
  link        bench_rcc     :    406 us

RCC -O1:
  preprocess  bench.c       :   8813 us
  parse       bench.c       :    541 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     42 us
  codegen     bench.c       :    402 us
  link        bench_o1      :    421 us

RCC -O2:
  preprocess  bench.c       :   9017 us
  parse       bench.c       :    566 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    319 us
  link        bench_o2      :    401 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 229714 us
  parse       sqlite3.c     : 123250 us
  typecheck   sqlite3.c     :   9398 us
  codegen     sqlite3.c     : 183829 us
  link        sqlite3.so    :   5889 us

RCC -O1:
  preprocess  sqlite3.c     : 233387 us
  parse       sqlite3.c     : 124119 us
  typecheck   sqlite3.c     :   9065 us
  opt         sqlite3.c     :  30729 us
  codegen     sqlite3.c     : 181583 us
  link        sqlite3.so    :   6241 us

RCC -O2:
  preprocess  sqlite3.c     : 229555 us
  parse       sqlite3.c     : 122686 us
  typecheck   sqlite3.c     :   9411 us
  opt         sqlite3.c     : 225289 us
  codegen     sqlite3.c     : 183370 us
  link        sqlite3.so    :   6380 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       884 ms |
| RCC -O1   |       895 ms |
| RCC -O2   |      1114 ms |
| TCC       |       107 ms |
| SLIMCC    |       712 ms |
| KEFIR     |     19440 ms |
| KEFIR -O1 |     34548 ms |
| ANTCC     |       408 ms |
| CCC       |     13063 ms |
| GCC -O0   |      4130 ms |
| GCC -O2   |     26717 ms |
| Clang -O0 |      1878 ms |
| Clang -O2 |     20409 ms |
