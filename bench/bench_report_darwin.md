# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          162 |          911 |       1073 |
| RCC -O1   |          123 |          945 |       1068 |
| RCC -O2   |          144 |          992 |       1136 |
| TCC       |          125 |          916 |       1041 |
| GCC -O0   |          148 |          693 |        841 |
| GCC -O2   |          197 |          369 |        566 |
| Clang -O0 |           90 |          599 |        689 |
| Clang -O2 |          118 |          299 |        417 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          244 |         6807 |       7051 |
| RCC -O1   |          353 |         5337 |       5690 |
| RCC -O2   |          263 |         4793 |       5056 |
| TCC       |          188 |         4901 |       5089 |
| GCC -O0   |          727 |         4265 |       4992 |
| GCC -O2   |         1100 |         2444 |       3544 |
| Clang -O0 |          629 |         4261 |       4890 |
| Clang -O2 |         1143 |         2249 |       3392 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    870 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    143 us
  link        bench_rcc     :    183 us
  link        bench_rcc     :  74364 us

RCC -O1:
  preprocess  bench.c       :    785 us
  parse       bench.c       :    277 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    153 us
  link        bench_o1      :    322 us
  link        bench_o1      :  89243 us

RCC -O2:
  preprocess  bench.c       :    652 us
  parse       bench.c       :    131 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    128 us
  link        bench_o2      :    468 us
  link        bench_o2      :  73466 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 458866 us
  parse       sqlite3.c     : 110869 us
  typecheck   sqlite3.c     :  24952 us
  codegen     sqlite3.c     : 167595 us
  link        sqlite3.so    :  28249 us

RCC -O1:
  preprocess  sqlite3.c     : 478517 us
  parse       sqlite3.c     : 115535 us
  typecheck   sqlite3.c     :  16323 us
  opt         sqlite3.c     : 315202 us
  codegen     sqlite3.c     : 293137 us
  link        sqlite3.so    :  23019 us

RCC -O2:
  preprocess  sqlite3.c     : 408956 us
  parse       sqlite3.c     : 142877 us
  typecheck   sqlite3.c     :  27070 us
  opt         sqlite3.c     : 361657 us
  codegen     sqlite3.c     : 278702 us
  link        sqlite3.so    :  22810 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1319 ms |
| RCC -O1   |      1094 ms |
| RCC -O2   |      1047 ms |
| TCC       |       167 ms |
| GCC -O0   |      1491 ms |
| GCC -O2   |     16103 ms |
| Clang -O0 |      2089 ms |
| Clang -O2 |     16122 ms |
