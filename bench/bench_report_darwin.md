# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           88 |          664 |        752 |
| RCC -O1   |           79 |          651 |        730 |
| RCC -O2   |           84 |          667 |        751 |
| TCC       |           62 |          592 |        654 |
| GCC -O0   |          109 |          471 |        580 |
| GCC -O2   |          129 |          297 |        426 |
| Clang -O0 |           76 |          486 |        562 |
| Clang -O2 |          161 |          288 |        449 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          177 |         5239 |       5416 |
| RCC -O1   |          226 |         5413 |       5639 |
| RCC -O2   |          381 |         5034 |       5415 |
| TCC       |          229 |         5102 |       5331 |
| GCC -O0   |          669 |         3873 |       4542 |
| GCC -O2   |         1038 |         2308 |       3346 |
| Clang -O0 |          659 |         3685 |       4344 |
| Clang -O2 |         1002 |         2124 |       3126 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    769 us
  parse       bench.c       :    183 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    175 us
  link        bench_rcc     :    278 us
  link        bench_rcc     :  79759 us

RCC -O1:
  preprocess  bench.c       :    893 us
  parse       bench.c       :    220 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    244 us
  link        bench_o1      :    201 us
  link        bench_o1      :  79738 us

RCC -O2:
  preprocess  bench.c       :    822 us
  parse       bench.c       :    147 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    146 us
  link        bench_o2      :    268 us
  link        bench_o2      :  81178 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 285436 us
  parse       sqlite3.c     :  75241 us
  typecheck   sqlite3.c     :  18810 us
  codegen     sqlite3.c     : 231429 us
  link        sqlite3.so    :  22484 us

RCC -O1:
  preprocess  sqlite3.c     : 387096 us
  parse       sqlite3.c     : 120350 us
  typecheck   sqlite3.c     :  18263 us
  opt         sqlite3.c     : 307055 us
  codegen     sqlite3.c     : 167805 us
  link        sqlite3.so    :  23349 us

RCC -O2:
  preprocess  sqlite3.c     : 362560 us
  parse       sqlite3.c     : 114154 us
  typecheck   sqlite3.c     :  19665 us
  opt         sqlite3.c     : 292042 us
  codegen     sqlite3.c     : 155705 us
  link        sqlite3.so    :  17780 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       888 ms |
| RCC -O1   |       915 ms |
| RCC -O2   |       872 ms |
| TCC       |       107 ms |
| GCC -O0   |      1114 ms |
| GCC -O2   |     11673 ms |
| Clang -O0 |      1280 ms |
| Clang -O2 |     11445 ms |
