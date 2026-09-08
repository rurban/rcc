# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           28 |          813 |        841 |    14% |
| RCC -O1   |           26 |          854 |        880 |    19% |
| RCC -O2   |           29 |          837 |        866 |     7% |
| TCC       |            5 |          568 |        573 |    40% |
| SLIMCC    |           34 |          578 |        612 |     8% |
| XCC       |            9 |          414 |        423 |    11% |
| KEFIR     |          204 |          661 |        865 |     8% |
| KEFIR -O1 |          242 |          356 |        598 |    12% |
| SCC       |           39 |          679 |        718 |    46% |
| LACC      |           25 |          899 |        924 |    23% |
| ANTCC     |           26 |          519 |        545 |    11% |
| CAKE      |          101 |          580 |        681 |    14% |
| CCC       |           37 |          639 |        676 |    45% |
| GCC -O0   |           59 |          547 |        606 |     5% |
| GCC -O2   |          170 |          201 |        371 |    12% |
| Clang -O0 |           87 |          530 |        617 |    16% |
| Clang -O2 |          158 |          204 |        362 |     6% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          418 |         7445 |       7863 |    12% |
| RCC -O1   |          438 |         8552 |       8990 |     3% |
| RCC -O2   |          425 |         7622 |       8047 |     3% |
| TCC       |           82 |         6674 |       6756 |    12% |
| SLIMCC    |          307 |         6585 |       6892 |     9% |
| KEFIR     |         5909 |         7011 |      12920 |     3% |
| KEFIR -O1 |         6178 |         5200 |      11378 |    17% |
| ANTCC     |          226 |         5551 |       5777 |    16% |
| CCC       |         1126 |         6478 |       7604 |    21% |
| GCC -O0   |         1092 |         6967 |       8059 |     8% |
| GCC -O2   |         2296 |         4033 |       6329 |    24% |
| Clang -O0 |         1097 |         6296 |       7393 |    10% |
| Clang -O2 |         2041 |         3839 |       5880 |     8% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  16086 us
  parse       bench.c       :    649 us
  typecheck   bench.c       :      8 us
  codegen     bench.c       :    329 us
  link        bench_rcc     :  14261 us

RCC -O1:
  preprocess  bench.c       :  14513 us
  parse       bench.c       :    644 us
  typecheck   bench.c       :      9 us
  opt         bench.c       :     50 us
  codegen     bench.c       :    361 us
  link        bench_o1      :  13402 us

RCC -O2:
  preprocess  bench.c       :  14012 us
  parse       bench.c       :    784 us
  typecheck   bench.c       :      7 us
  opt         bench.c       :     35 us
  codegen     bench.c       :    328 us
  link        bench_o2      :  13904 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 402786 us
  parse       sqlite3.c     : 226258 us
  typecheck   sqlite3.c     :  11527 us
  codegen     sqlite3.c     : 321628 us
  link        sqlite3.so    :  15219 us

RCC -O1:
  preprocess  sqlite3.c     : 345529 us
  parse       sqlite3.c     : 190388 us
  typecheck   sqlite3.c     :   9311 us
  opt         sqlite3.c     : 272891 us
  codegen     sqlite3.c     : 250852 us
  link        sqlite3.so    :  13411 us

RCC -O2:
  preprocess  sqlite3.c     : 288460 us
  parse       sqlite3.c     : 173635 us
  typecheck   sqlite3.c     :   9081 us
  opt         sqlite3.c     : 280308 us
  codegen     sqlite3.c     : 253905 us
  link        sqlite3.so    :  14449 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |      1416 ms |     2% |
| RCC -O1   |      1497 ms |     0% |
| RCC -O2   |      1530 ms |     1% |
| TCC       |       131 ms |     6% |
| SLIMCC    |       966 ms |     7% |
| KEFIR     |     25243 ms |     7% |
| KEFIR -O1 |     46863 ms |     0% |
| ANTCC     |       508 ms |     0% |
| CCC       |     16189 ms |     2% |
| GCC -O0   |      5265 ms |     2% |
| GCC -O2   |     32678 ms |     0% |
| Clang -O0 |      2316 ms |     0% |
| Clang -O2 |     25328 ms |     0% |
