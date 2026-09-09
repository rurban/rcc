# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           54 |          659 |        713 |    25% |
| RCC -O1   |           62 |          632 |        694 |    16% |
| RCC -O2   |           53 |          616 |        669 |    25% |
| TCC       |           25 |          530 |        555 |    72% |
| GCC -O0   |           66 |          472 |        538 |     8% |
| GCC -O2   |           93 |          291 |        384 |    34% |
| Clang -O0 |           59 |          469 |        528 |    27% |
| Clang -O2 |          116 |          296 |        412 |    25% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          130 |         5004 |       5134 |    25% |
| RCC -O1   |          134 |         4905 |       5039 |    31% |
| RCC -O2   |          127 |         4529 |       4656 |   104% |
| TCC       |          103 |         3828 |       3931 |    52% |
| GCC -O0   |          475 |         3099 |       3574 |     9% |
| GCC -O2   |          803 |         2284 |       3087 |    17% |
| Clang -O0 |          626 |         3993 |       4619 |    51% |
| Clang -O2 |          914 |         2076 |       2990 |    22% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    851 us
  parse       bench.c       :    189 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    185 us
  link        bench_rcc     :    149 us
  link        bench_rcc     :  71174 us

RCC -O1:
  preprocess  bench.c       :    644 us
  parse       bench.c       :    151 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    142 us
  link        bench_o1      :    159 us
  link        bench_o1      :  55668 us

RCC -O2:
  preprocess  bench.c       :    637 us
  parse       bench.c       :    155 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    139 us
  link        bench_o2      :    141 us
  link        bench_o2      :  51975 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 216949 us
  parse       sqlite3.c     :  71658 us
  typecheck   sqlite3.c     :  23452 us
  codegen     sqlite3.c     : 103313 us
  link        sqlite3.so    :  20200 us

RCC -O1:
  preprocess  sqlite3.c     : 209657 us
  parse       sqlite3.c     :  72320 us
  typecheck   sqlite3.c     :  18611 us
  opt         sqlite3.c     :  26763 us
  codegen     sqlite3.c     : 124792 us
  link        sqlite3.so    :  17078 us

RCC -O2:
  preprocess  sqlite3.c     : 231339 us
  parse       sqlite3.c     :  53456 us
  typecheck   sqlite3.c     :  14081 us
  opt         sqlite3.c     :  37512 us
  codegen     sqlite3.c     : 114482 us
  link        sqlite3.so    :  26955 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       612 ms |    26% |
| RCC -O1   |       592 ms |     2% |
| RCC -O2   |       547 ms |    19% |
| TCC       |       112 ms |    31% |
| GCC -O0   |      1204 ms |     1% |
| GCC -O2   |     10544 ms |     7% |
| Clang -O0 |      1026 ms |     6% |
| Clang -O2 |      9648 ms |     0% |
