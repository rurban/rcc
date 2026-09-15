# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           14 |          700 |        714 |     7% |
| RCC -O1   |           14 |          694 |        708 |     7% |
| RCC -O2   |           14 |          693 |        707 |     7% |
| TCC       |            6 |          488 |        494 |     0% |
| SLIMCC    |           34 |          498 |        532 |    67% |
| XCC       |            9 |          357 |        366 |   255% |
| KEFIR     |          183 |          571 |        754 |     6% |
| KEFIR -O1 |          191 |          306 |        497 |     1% |
| SCC       |           35 |          537 |        572 |   277% |
| LACC      |           28 |          756 |        784 |    53% |
| ANTCC     |           26 |          414 |        440 |     0% |
| CAKE      |           94 |          478 |        572 |    17% |
| BCC       |           29 |          556 |        585 |     3% |
| CCC       |           34 |          536 |        570 |    13% |
| GCC -O0   |           58 |          477 |        535 |     1% |
| GCC -O2   |          152 |          175 |        327 |    18% |
| Clang -O0 |           84 |          463 |        547 |   971% |
| Clang -O2 |          131 |          176 |        307 |    78% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          307 |         5817 |       6124 |     7% |
| RCC -O1   |          304 |         6545 |       6849 |     4% |
| RCC -O2   |          303 |         6102 |       6405 |     4% |
| TCC       |           68 |         5038 |       5106 |     1% |
| SLIMCC    |          256 |         4922 |       5178 |     3% |
| KEFIR     |         4388 |         5269 |       9657 |     0% |
| KEFIR -O1 |         4824 |         3692 |       8516 |     0% |
| ANTCC     |          191 |         3731 |       3922 |     2% |
| CCC       |          869 |         4467 |       5336 |     0% |
| BCC       |         2965 |         4678 |       7643 |     3% |
| GCC -O0   |          843 |         4814 |       5657 |     2% |
| GCC -O2   |         1853 |         2520 |       4373 |     1% |
| Clang -O0 |          874 |         4569 |       5443 |     3% |
| Clang -O2 |         1587 |         2554 |       4141 |     3% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   8736 us
  parse       bench.c       :    613 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    354 us
  link        bench_rcc     :   1304 us

RCC -O1:
  preprocess  bench.c       :   9228 us
  parse       bench.c       :    629 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     46 us
  codegen     bench.c       :    306 us
  link        bench_o1      :   1295 us

RCC -O2:
  preprocess  bench.c       :   9346 us
  parse       bench.c       :    642 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     48 us
  codegen     bench.c       :    308 us
  link        bench_o2      :   1327 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 218392 us
  parse       sqlite3.c     : 149996 us
  typecheck   sqlite3.c     :   8212 us
  codegen     sqlite3.c     : 204198 us
  link        sqlite3.so    :  10147 us

RCC -O1:
  preprocess  sqlite3.c     : 220042 us
  parse       sqlite3.c     : 147583 us
  typecheck   sqlite3.c     :   8181 us
  opt         sqlite3.c     :  36939 us
  codegen     sqlite3.c     : 199704 us
  link        sqlite3.so    :  10703 us

RCC -O2:
  preprocess  sqlite3.c     : 215922 us
  parse       sqlite3.c     : 147086 us
  typecheck   sqlite3.c     :   8213 us
  opt         sqlite3.c     :  46294 us
  codegen     sqlite3.c     : 201575 us
  link        sqlite3.so    :  10755 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       686 ms |     1% |
| RCC -O1   |       697 ms |     0% |
| RCC -O2   |       710 ms |     0% |
| TCC       |        98 ms |     2% |
| SLIMCC    |       656 ms |     2% |
| KEFIR     |     18277 ms |     0% |
| KEFIR -O1 |     33080 ms |     0% |
| ANTCC     |       398 ms |     0% |
| CCC       |     12615 ms |     0% |
| GCC -O0   |      4027 ms |     1% |
| GCC -O2   |     25661 ms |     0% |
| Clang -O0 |      1807 ms |     1% |
| Clang -O2 |     19913 ms |     0% |
