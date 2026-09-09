# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          109 |          835 |        944 |   114% |
| RCC -O1   |           81 |          672 |        753 |    90% |
| RCC -O2   |           62 |          699 |        761 |    11% |
| TCC       |           32 |          675 |        707 |    75% |
| GCC -O0   |           78 |          529 |        607 |    51% |
| GCC -O2   |          114 |          316 |        430 |    78% |
| Clang -O0 |           69 |          562 |        631 |    27% |
| Clang -O2 |          109 |          324 |        433 |    28% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          176 |         6985 |       7161 |    82% |
| RCC -O1   |          173 |         6345 |       6518 |   153% |
| RCC -O2   |          179 |         4638 |       4817 |   113% |
| TCC       |          117 |         4693 |       4810 |    43% |
| GCC -O0   |          619 |         3617 |       4236 |    32% |
| GCC -O2   |         1052 |         2501 |       3553 |    31% |
| Clang -O0 |          587 |         4060 |       4647 |    29% |
| Clang -O2 |         1144 |         2476 |       3620 |    22% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    796 us
  parse       bench.c       :    195 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    175 us
  link        bench_rcc     :    288 us
  link        bench_rcc     :  58208 us

RCC -O1:
  preprocess  bench.c       :    673 us
  parse       bench.c       :    151 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    140 us
  link        bench_o1      :    307 us
  link        bench_o1      :  52814 us

RCC -O2:
  preprocess  bench.c       :    616 us
  parse       bench.c       :    215 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    140 us
  link        bench_o2      :    131 us
  link        bench_o2      :  60258 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 552675 us
  parse       sqlite3.c     : 252200 us
  typecheck   sqlite3.c     :  22950 us
  codegen     sqlite3.c     : 237425 us
  link        sqlite3.so    :  19940 us

RCC -O1:
  preprocess  sqlite3.c     : 435669 us
  parse       sqlite3.c     : 102398 us
  typecheck   sqlite3.c     :  35881 us
  opt         sqlite3.c     :  37818 us
  codegen     sqlite3.c     : 235564 us
  link        sqlite3.so    :  25672 us

RCC -O2:
  preprocess  sqlite3.c     : 413352 us
  parse       sqlite3.c     :  94260 us
  typecheck   sqlite3.c     :  21947 us
  opt         sqlite3.c     :  50998 us
  codegen     sqlite3.c     : 250712 us
  link        sqlite3.so    :  65485 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       588 ms |    55% |
| RCC -O1   |       521 ms |     3% |
| RCC -O2   |       595 ms |     2% |
| TCC       |       125 ms |     1% |
| GCC -O0   |      1144 ms |    14% |
| GCC -O2   |     11531 ms |    20% |
| Clang -O0 |      1239 ms |    21% |
| Clang -O2 |     11852 ms |    11% |
