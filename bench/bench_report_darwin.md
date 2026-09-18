# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           62 |          767 |        829 |    58% |
| RCC -O1   |           72 |          731 |        803 |    19% |
| RCC -O2   |           77 |          789 |        866 |    66% |
| TCC       |           73 |          759 |        832 |   147% |
| GCC -O0   |          103 |          608 |        711 |    85% |
| GCC -O2   |          146 |          309 |        455 |    89% |
| Clang -O0 |          115 |          568 |        683 |    56% |
| Clang -O2 |           98 |          327 |        425 |   102% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          209 |         6264 |       6473 |    40% |
| RCC -O1   |          214 |         6327 |       6541 |     7% |
| RCC -O2   |          128 |         5133 |       5261 |   128% |
| TCC       |          100 |         4730 |       4830 |   110% |
| GCC -O0   |          498 |         3371 |       3869 |    48% |
| GCC -O2   |          796 |         1799 |       2595 |    55% |
| Clang -O0 |          462 |         3648 |       4110 |    21% |
| Clang -O2 |          886 |         2192 |       3078 |    37% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1319 us
  parse       bench.c       :    169 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    158 us
  link        bench_rcc     :    209 us
  link        bench_rcc     :  85016 us

RCC -O1:
  preprocess  bench.c       :    641 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     50 us
  codegen     bench.c       :    135 us
  link        bench_o1      :    188 us
  link        bench_o1      :  86234 us

RCC -O2:
  preprocess  bench.c       :    697 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     54 us
  codegen     bench.c       :    144 us
  link        bench_o2      :    241 us
  link        bench_o2      :  79940 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 306871 us
  parse       sqlite3.c     : 222720 us
  typecheck   sqlite3.c     :  23843 us
  codegen     sqlite3.c     : 158327 us
  link        sqlite3.so    :  19655 us

RCC -O1:
  preprocess  sqlite3.c     : 342340 us
  parse       sqlite3.c     :  63960 us
  typecheck   sqlite3.c     :  15377 us
  opt         sqlite3.c     :  52119 us
  codegen     sqlite3.c     : 210449 us
  link        sqlite3.so    :  18310 us

RCC -O2:
  preprocess  sqlite3.c     : 282634 us
  parse       sqlite3.c     : 109442 us
  typecheck   sqlite3.c     :  13289 us
  opt         sqlite3.c     :  58142 us
  codegen     sqlite3.c     : 218527 us
  link        sqlite3.so    :  17383 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       442 ms |    76% |
| RCC -O1   |       487 ms |    24% |
| RCC -O2   |       505 ms |     0% |
| TCC       |        87 ms |    37% |
| GCC -O0   |       929 ms |    15% |
| GCC -O2   |      9035 ms |     3% |
| Clang -O0 |       963 ms |    39% |
| Clang -O2 |      8762 ms |     3% |
