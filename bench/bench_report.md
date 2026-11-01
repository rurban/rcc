# Linux RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           15 |          785 |        800 |     4% |
| RCC -O1   |           15 |          781 |        796 |     0% |
| RCC -O2   |           15 |          774 |        789 |     9% |
| TCC       |            5 |          551 |        556 |   280% |
| SLIMCC    |           34 |          566 |        600 |    50% |
| XCC       |            9 |          404 |        413 |   266% |
| KEFIR     |          192 |          648 |        840 |    40% |
| KEFIR -O1 |          202 |          346 |        548 |     8% |
| SCC       |           35 |          616 |        651 |   277% |
| LACC      |           23 |          853 |        876 |    56% |
| ANTCC     |           25 |          473 |        498 |    12% |
| CAKE      |           93 |          543 |        636 |    21% |
| BCC       |           29 |          634 |        663 |   203% |
| CCC       |           34 |          607 |        641 |    16% |
| GCC -O0   |           58 |          534 |        592 |     1% |
| GCC -O2   |          160 |          192 |        352 |    18% |
| Clang -O0 |           82 |          518 |        600 |    12% |
| Clang -O2 |          140 |          199 |        339 |     8% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          308 |         6827 |       7135 |    12% |
| RCC -O1   |          321 |         7484 |       7805 |     9% |
| RCC -O2   |          309 |         6873 |       7182 |     2% |
| TCC       |           72 |         5662 |       5734 |    11% |
| SLIMCC    |          273 |         5535 |       5808 |     3% |
| KEFIR     |         4910 |         5947 |      10857 |     0% |
| KEFIR -O1 |         5344 |         4169 |       9513 |     0% |
| ANTCC     |          201 |         4194 |       4395 |     2% |
| CCC       |          956 |         5033 |       5989 |     1% |
| BCC       |         3265 |         5138 |       8403 |     3% |
| GCC -O0   |          923 |         5420 |       6343 |     0% |
| GCC -O2   |         2053 |         2857 |       4910 |     3% |
| Clang -O0 |          942 |         5158 |       6100 |     0% |
| Clang -O2 |         1743 |         2907 |       4650 |     2% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :  11386 us
  parse       bench.c       :    653 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    321 us
  link        bench_rcc     :   2456 us

RCC -O1:
  preprocess  bench.c       :  10503 us
  parse       bench.c       :    580 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     86 us
  codegen     bench.c       :    358 us
  link        bench_o1      :    854 us

RCC -O2:
  preprocess  bench.c       :  10389 us
  parse       bench.c       :    687 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    149 us
  codegen     bench.c       :    361 us
  link        bench_o2      :    899 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 284108 us
  parse       sqlite3.c     : 166010 us
  typecheck   sqlite3.c     :   8536 us
  codegen     sqlite3.c     : 232257 us
  link        sqlite3.so    :   8753 us

RCC -O1:
  preprocess  sqlite3.c     : 274922 us
  parse       sqlite3.c     : 164493 us
  typecheck   sqlite3.c     :   8678 us
  opt         sqlite3.c     :  70737 us
  codegen     sqlite3.c     : 230380 us
  link        sqlite3.so    :   9418 us

RCC -O2:
  preprocess  sqlite3.c     : 274915 us
  parse       sqlite3.c     : 161203 us
  typecheck   sqlite3.c     :   8537 us
  opt         sqlite3.c     :  98737 us
  codegen     sqlite3.c     : 233471 us
  link        sqlite3.so    :   9011 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       800 ms |     0% |
| RCC -O1   |       835 ms |     0% |
| RCC -O2   |       866 ms |     0% |
| TCC       |       108 ms |     6% |
| SLIMCC    |       723 ms |     1% |
| KEFIR     |     20567 ms |     0% |
| KEFIR -O1 |     37217 ms |     0% |
| ANTCC     |       434 ms |     0% |
| CCC       |     14286 ms |     0% |
| GCC -O0   |      4502 ms |     1% |
| GCC -O2   |     28729 ms |     0% |
| Clang -O0 |      1997 ms |     0% |
| Clang -O2 |     22285 ms |     0% |
