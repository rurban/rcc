# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           88 |          924 |       1012 |    81% |
| RCC -O1   |          109 |          799 |        908 |    18% |
| RCC -O2   |           97 |          730 |        827 |    21% |
| TCC       |           45 |          666 |        711 |   157% |
| GCC -O0   |           94 |          617 |        711 |    56% |
| GCC -O2   |          180 |          344 |        524 |    27% |
| Clang -O0 |           85 |          624 |        709 |    69% |
| Clang -O2 |          133 |          335 |        468 |    17% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          245 |         7165 |       7410 |    29% |
| RCC -O1   |          199 |         7222 |       7421 |   149% |
| RCC -O2   |          191 |         7013 |       7204 |    52% |
| TCC       |          173 |         5699 |       5872 |   131% |
| GCC -O0   |          563 |         3977 |       4540 |    44% |
| GCC -O2   |          947 |         3100 |       4047 |    20% |
| Clang -O0 |          533 |         3876 |       4409 |    52% |
| Clang -O2 |          926 |         2446 |       3372 |    14% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    832 us
  parse       bench.c       :    196 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    176 us
  link        bench_rcc     :    304 us
  link        bench_rcc     :  83183 us

RCC -O1:
  preprocess  bench.c       :    716 us
  parse       bench.c       :    177 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    219 us
  link        bench_o1      :    909 us
  link        bench_o1      :  86179 us

RCC -O2:
  preprocess  bench.c       :    657 us
  parse       bench.c       :    203 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    140 us
  link        bench_o2      :    147 us
  link        bench_o2      :  74880 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 312468 us
  parse       sqlite3.c     : 115576 us
  typecheck   sqlite3.c     :  20538 us
  codegen     sqlite3.c     : 210618 us
  link        sqlite3.so    :  31725 us

RCC -O1:
  preprocess  sqlite3.c     : 299198 us
  parse       sqlite3.c     :  69485 us
  typecheck   sqlite3.c     :  14931 us
  opt         sqlite3.c     :  45780 us
  codegen     sqlite3.c     : 149486 us
  link        sqlite3.so    :  18998 us

RCC -O2:
  preprocess  sqlite3.c     : 324838 us
  parse       sqlite3.c     :  78789 us
  typecheck   sqlite3.c     :  14906 us
  opt         sqlite3.c     :  43227 us
  codegen     sqlite3.c     : 163463 us
  link        sqlite3.so    :  19323 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       651 ms |    83% |
| RCC -O1   |       501 ms |     6% |
| RCC -O2   |       466 ms |    17% |
| TCC       |        92 ms |    84% |
| GCC -O0   |      1185 ms |    13% |
| GCC -O2   |     11502 ms |     6% |
| Clang -O0 |      1390 ms |    14% |
| Clang -O2 |     13916 ms |     8% |
