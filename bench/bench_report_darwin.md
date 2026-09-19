# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           55 |          834 |        889 |    20% |
| RCC -O1   |           47 |          806 |        853 |    21% |
| RCC -O2   |           63 |          783 |        846 |    33% |
| TCC       |           49 |          678 |        727 |    85% |
| GCC -O0   |          127 |          573 |        700 |    55% |
| GCC -O2   |          113 |          340 |        453 |    71% |
| Clang -O0 |           80 |          586 |        666 |    48% |
| Clang -O2 |          157 |          345 |        502 |    40% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          182 |         6380 |       6562 |    71% |
| RCC -O1   |          194 |         5684 |       5878 |    52% |
| RCC -O2   |          179 |         5768 |       5947 |    93% |
| TCC       |          181 |         4360 |       4541 |    55% |
| GCC -O0   |          476 |         3317 |       3793 |    34% |
| GCC -O2   |          980 |         1861 |       2841 |    24% |
| Clang -O0 |          456 |         3497 |       3953 |     7% |
| Clang -O2 |          920 |         2348 |       3268 |    18% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    795 us
  parse       bench.c       :    189 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    196 us
  link        bench_rcc     :  29800 us

RCC -O1:
  preprocess  bench.c       :    607 us
  parse       bench.c       :    171 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     48 us
  codegen     bench.c       :    142 us
  link        bench_o1      :  32584 us

RCC -O2:
  preprocess  bench.c       :    707 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     50 us
  codegen     bench.c       :    137 us
  link        bench_o2      :  26704 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 318023 us
  parse       sqlite3.c     : 211674 us
  typecheck   sqlite3.c     :  25864 us
  codegen     sqlite3.c     : 135928 us
  link        sqlite3.so    :  90381 us

RCC -O1:
  preprocess  sqlite3.c     : 303375 us
  parse       sqlite3.c     :  73648 us
  typecheck   sqlite3.c     :  14269 us
  opt         sqlite3.c     :  77294 us
  codegen     sqlite3.c     : 191087 us
  link        sqlite3.so    :  54016 us

RCC -O2:
  preprocess  sqlite3.c     : 338307 us
  parse       sqlite3.c     :  95330 us
  typecheck   sqlite3.c     :  22924 us
  opt         sqlite3.c     :  84686 us
  codegen     sqlite3.c     : 211776 us
  link        sqlite3.so    :  47786 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       599 ms |    62% |
| RCC -O1   |       463 ms |    10% |
| RCC -O2   |       445 ms |     3% |
| TCC       |       102 ms |     7% |
| GCC -O0   |       958 ms |    12% |
| GCC -O2   |      9464 ms |     1% |
| Clang -O0 |      1002 ms |     5% |
| Clang -O2 |      9336 ms |     1% |
