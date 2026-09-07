# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          137 |          815 |        952 |
| RCC -O1   |          101 |          837 |        938 |
| RCC -O2   |           97 |          819 |        916 |
| TCC       |           98 |          723 |        821 |
| GCC -O0   |          124 |          615 |        739 |
| GCC -O2   |          157 |          339 |        496 |
| Clang -O0 |          117 |          646 |        763 |
| Clang -O2 |          266 |          364 |        630 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          274 |         7088 |       7362 |
| RCC -O1   |          316 |         7431 |       7747 |
| RCC -O2   |          273 |         4726 |       4999 |
| TCC       |          187 |         5258 |       5445 |
| GCC -O0   |          677 |         3863 |       4540 |
| GCC -O2   |         1150 |         2696 |       3846 |
| Clang -O0 |          791 |         4123 |       4914 |
| Clang -O2 |         1194 |         2281 |       3475 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    851 us
  parse       bench.c       :    213 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    187 us
  link        bench_rcc     :    193 us
  link        bench_rcc     :  74794 us

RCC -O1:
  preprocess  bench.c       :    753 us
  parse       bench.c       :    187 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    163 us
  link        bench_o1      :    476 us
  link        bench_o1      :  67740 us

RCC -O2:
  preprocess  bench.c       :   1120 us
  parse       bench.c       :    217 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    196 us
  link        bench_o2      :    136 us
  link        bench_o2      :  74012 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 420416 us
  parse       sqlite3.c     : 103618 us
  typecheck   sqlite3.c     :  23741 us
  codegen     sqlite3.c     : 274665 us
  link        sqlite3.so    :  24365 us

RCC -O1:
  preprocess  sqlite3.c     : 356420 us
  parse       sqlite3.c     :  89588 us
  typecheck   sqlite3.c     :  27374 us
  opt         sqlite3.c     : 264022 us
  codegen     sqlite3.c     : 223481 us
  link        sqlite3.so    :  23413 us

RCC -O2:
  preprocess  sqlite3.c     : 389385 us
  parse       sqlite3.c     :  84754 us
  typecheck   sqlite3.c     :  15436 us
  opt         sqlite3.c     : 268375 us
  codegen     sqlite3.c     : 171086 us
  link        sqlite3.so    :  21455 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1164 ms |
| RCC -O1   |       812 ms |
| RCC -O2   |       788 ms |
| TCC       |       136 ms |
| GCC -O0   |      1228 ms |
| GCC -O2   |     13298 ms |
| Clang -O0 |      1212 ms |
| Clang -O2 |     11559 ms |
