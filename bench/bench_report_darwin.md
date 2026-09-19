# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           66 |          694 |        760 |    77% |
| RCC -O1   |           64 |          661 |        725 |    23% |
| RCC -O2   |           67 |          695 |        762 |    10% |
| TCC       |           33 |          564 |        597 |    54% |
| GCC -O0   |           80 |          462 |        542 |    33% |
| GCC -O2   |           97 |          279 |        376 |    49% |
| Clang -O0 |           70 |          491 |        561 |     7% |
| Clang -O2 |          112 |          286 |        398 |    17% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          179 |         5230 |       5409 |    46% |
| RCC -O1   |          166 |         5989 |       6155 |    39% |
| RCC -O2   |          155 |         4374 |       4529 |   133% |
| TCC       |          107 |         4421 |       4528 |    54% |
| GCC -O0   |          568 |         3279 |       3847 |    18% |
| GCC -O2   |          908 |         1986 |       2894 |    22% |
| Clang -O0 |          529 |         3368 |       3897 |     8% |
| Clang -O2 |          814 |         1870 |       2684 |    12% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2775 us
  parse       bench.c       :   1155 us
  typecheck   bench.c       :      9 us
  codegen     bench.c       :    420 us
  link        bench_rcc     :    516 us
  link        bench_rcc     :  85921 us

RCC -O1:
  preprocess  bench.c       :    734 us
  parse       bench.c       :    222 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     55 us
  codegen     bench.c       :    201 us
  link        bench_o1      :    192 us
  link        bench_o1      :  77552 us

RCC -O2:
  preprocess  bench.c       :    790 us
  parse       bench.c       :    245 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     67 us
  codegen     bench.c       :    238 us
  link        bench_o2      :    219 us
  link        bench_o2      :  86614 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 321647 us
  parse       sqlite3.c     :  76516 us
  typecheck   sqlite3.c     :  13843 us
  codegen     sqlite3.c     : 157875 us
  link        sqlite3.so    :  18189 us

RCC -O1:
  preprocess  sqlite3.c     : 256679 us
  parse       sqlite3.c     :  63815 us
  typecheck   sqlite3.c     :  16306 us
  opt         sqlite3.c     :  54306 us
  codegen     sqlite3.c     : 159478 us
  link        sqlite3.so    :  20025 us

RCC -O2:
  preprocess  sqlite3.c     : 271077 us
  parse       sqlite3.c     :  71915 us
  typecheck   sqlite3.c     :  14844 us
  opt         sqlite3.c     :  81864 us
  codegen     sqlite3.c     : 154539 us
  link        sqlite3.so    :  16158 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       479 ms |    41% |
| RCC -O1   |       505 ms |     1% |
| RCC -O2   |       444 ms |     0% |
| TCC       |        86 ms |     8% |
| GCC -O0   |       908 ms |     3% |
| GCC -O2   |      8929 ms |     1% |
| Clang -O0 |       906 ms |     1% |
| Clang -O2 |      8668 ms |     1% |
