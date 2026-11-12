# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           24 |          555 |        579 |     4% |
| RCC -O1   |           26 |          556 |        582 |    11% |
| RCC -O2   |           26 |          293 |        319 |     6% |
| RCC -O3   |           30 |          292 |        322 |    30% |
| GCC -O0   |           45 |          396 |        441 |    88% |
| GCC -O2   |           66 |          236 |        302 |    84% |
| Clang -O0 |           40 |          396 |        436 |     2% |
| Clang -O2 |           60 |          236 |        296 |     3% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          105 |         4076 |       4181 |    20% |
| RCC -O1   |          104 |         4097 |       4201 |    10% |
| RCC -O2   |          129 |         1907 |       2036 |    24% |
| RCC -O3   |          130 |         1912 |       2042 |     5% |
| GCC -O0   |          322 |         2948 |       3270 |     8% |
| GCC -O2   |          580 |         1685 |       2265 |    39% |
| Clang -O0 |          319 |         2941 |       3260 |     4% |
| Clang -O2 |          578 |         1685 |       2263 |     1% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   3573 us
  parse       bench.c       :   1576 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :   1168 us
  link        bench_rcc     :  21853 us

RCC -O1:
  preprocess  bench.c       :    443 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     52 us
  codegen     bench.c       :    103 us
  link        bench_o1      :  22441 us

RCC -O2:
  preprocess  bench.c       :    436 us
  parse       bench.c       :    120 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     42 us
  codegen     bench.c       :   1281 us
  link        bench_o2      :  21651 us

RCC -O3:
  preprocess  bench.c       :    429 us
  parse       bench.c       :    122 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     42 us
  codegen     bench.c       :   1073 us
  link        bench_o3      :  19814 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 186176 us
  parse       sqlite3.c     :  46252 us
  typecheck   sqlite3.c     :  14285 us
  codegen     sqlite3.c     :  88538 us
  link        sqlite3.so    :  23206 us

RCC -O1:
  preprocess  sqlite3.c     : 148099 us
  parse       sqlite3.c     :  44040 us
  typecheck   sqlite3.c     :  14885 us
  opt         sqlite3.c     :  38958 us
  codegen     sqlite3.c     :  91133 us
  link        sqlite3.so    :  22696 us

RCC -O2:
  preprocess  sqlite3.c     : 148565 us
  parse       sqlite3.c     :  41853 us
  typecheck   sqlite3.c     :   9849 us
  opt         sqlite3.c     :  42529 us
  codegen     sqlite3.c     : 859121 us
  link        sqlite3.so    :  18388 us

RCC -O3:
  preprocess  sqlite3.c     : 147779 us
  parse       sqlite3.c     :  41063 us
  typecheck   sqlite3.c     :   9832 us
  opt         sqlite3.c     :  42226 us
  codegen     sqlite3.c     : 854383 us
  link        sqlite3.so    :  18559 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       361 ms |    24% |
| RCC -O1   |       384 ms |     1% |
| RCC -O2   |      1163 ms |     1% |
| RCC -O3   |      1146 ms |     0% |
| GCC -O0   |       745 ms |     8% |
| GCC -O2   |      7409 ms |     2% |
| Clang -O0 |       748 ms |     1% |
| Clang -O2 |      7431 ms |     0% |
