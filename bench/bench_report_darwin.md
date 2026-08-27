# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           61 |          599 |        660 |
| RCC -O1   |           48 |          591 |        639 |
| RCC -O2   |           48 |          594 |        642 |
| TCC       |           40 |          514 |        554 |
| GCC -O0   |           58 |          434 |        492 |
| GCC -O2   |          100 |          263 |        363 |
| Clang -O0 |           51 |          433 |        484 |
| Clang -O2 |           78 |          263 |        341 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    623 us
  parse       bench.c       :    141 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    118 us
  link        bench_rcc     :    111 us
  link        bench_rcc     :  48160 us

RCC -O1:
  preprocess  bench.c       :    597 us
  parse       bench.c       :    142 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    116 us
  link        bench_o1      :    354 us
  link        bench_o1      :  42959 us

RCC -O2:
  preprocess  bench.c       :    551 us
  parse       bench.c       :    121 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    115 us
  link        bench_o2      :    436 us
  link        bench_o2      :  43451 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 179086 us
  parse       sqlite3.c     :  52559 us
  typecheck   sqlite3.c     :  16057 us
  codegen     sqlite3.c     :  88976 us
  link        sqlite3.so    :  13398 us

RCC -O1:
  preprocess  sqlite3.c     : 185838 us
  parse       sqlite3.c     :  49676 us
  typecheck   sqlite3.c     :  13214 us
  opt         sqlite3.c     : 135260 us
  codegen     sqlite3.c     :  96760 us
  link        sqlite3.so    :  14923 us

RCC -O2:
  preprocess  sqlite3.c     : 173321 us
  parse       sqlite3.c     :  44388 us
  typecheck   sqlite3.c     :  11726 us
  opt         sqlite3.c     : 122824 us
  codegen     sqlite3.c     :  86702 us
  link        sqlite3.so    :  14390 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       520 ms |
| RCC -O1   |       666 ms |
| RCC -O2   |       637 ms |
| TCC       |        86 ms |
| GCC -O0   |       910 ms |
| GCC -O2   |      9129 ms |
| Clang -O0 |       918 ms |
| Clang -O2 |      9116 ms |
