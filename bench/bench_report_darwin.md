# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           54 |          650 |        704 |
| RCC -O1   |           59 |          668 |        727 |
| RCC -O2   |           58 |          651 |        709 |
| TCC       |           45 |          571 |        616 |
| GCC -O0   |           73 |          518 |        591 |
| GCC -O2   |          124 |          302 |        426 |
| Clang -O0 |           62 |          496 |        558 |
| Clang -O2 |           88 |          303 |        391 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          165 |         5110 |       5275 |
| RCC -O1   |          183 |         5329 |       5512 |
| RCC -O2   |          220 |         4615 |       4835 |
| TCC       |          137 |         4218 |       4355 |
| GCC -O0   |          534 |         3403 |       3937 |
| GCC -O2   |          923 |         1931 |       2854 |
| Clang -O0 |          492 |         3415 |       3907 |
| Clang -O2 |          896 |         1967 |       2863 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1549 us
  parse       bench.c       :    217 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    159 us
  link        bench_rcc     :    258 us
  link        bench_rcc     :  73022 us

RCC -O1:
  preprocess  bench.c       :    747 us
  parse       bench.c       :    178 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    217 us
  link        bench_o1      :    203 us
  link        bench_o1      :  68413 us

RCC -O2:
  preprocess  bench.c       :    731 us
  parse       bench.c       :    249 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    162 us
  link        bench_o2      :    229 us
  link        bench_o2      :  68653 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 329353 us
  parse       sqlite3.c     :  81427 us
  typecheck   sqlite3.c     :  12821 us
  codegen     sqlite3.c     : 112503 us
  link        sqlite3.so    :  16099 us

RCC -O1:
  preprocess  sqlite3.c     : 223332 us
  parse       sqlite3.c     :  55762 us
  typecheck   sqlite3.c     :  11950 us
  opt         sqlite3.c     : 153901 us
  codegen     sqlite3.c     : 112945 us
  link        sqlite3.so    :  16156 us

RCC -O2:
  preprocess  sqlite3.c     : 202806 us
  parse       sqlite3.c     :  53207 us
  typecheck   sqlite3.c     :  11895 us
  opt         sqlite3.c     : 148353 us
  codegen     sqlite3.c     :  96634 us
  link        sqlite3.so    :  16178 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       715 ms |
| RCC -O1   |       726 ms |
| RCC -O2   |       721 ms |
| TCC       |        96 ms |
| GCC -O0   |      1008 ms |
| GCC -O2   |     10335 ms |
| Clang -O0 |      1001 ms |
| Clang -O2 |     11946 ms |
