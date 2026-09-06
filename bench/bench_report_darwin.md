# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           82 |          647 |        729 |
| RCC -O1   |           52 |          645 |        697 |
| RCC -O2   |           62 |          641 |        703 |
| TCC       |           46 |          545 |        591 |
| GCC -O0   |          118 |          434 |        552 |
| GCC -O2   |           99 |          264 |        363 |
| Clang -O0 |           58 |          436 |        494 |
| Clang -O2 |          110 |          271 |        381 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          230 |         5550 |       5780 |
| RCC -O1   |          298 |         5208 |       5506 |
| RCC -O2   |          214 |         4467 |       4681 |
| TCC       |          184 |         4155 |       4339 |
| GCC -O0   |          610 |         3603 |       4213 |
| GCC -O2   |          958 |         1990 |       2948 |
| Clang -O0 |          514 |         3467 |       3981 |
| Clang -O2 |         1096 |         1881 |       2977 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    650 us
  parse       bench.c       :    135 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    119 us
  link        bench_rcc     :     81 us
  link        bench_rcc     :  44239 us

RCC -O1:
  preprocess  bench.c       :    566 us
  parse       bench.c       :    136 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    122 us
  link        bench_o1      :    107 us
  link        bench_o1      :  42901 us

RCC -O2:
  preprocess  bench.c       :    564 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    124 us
  link        bench_o2      :    169 us
  link        bench_o2      :  47071 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 210078 us
  parse       sqlite3.c     :  52749 us
  typecheck   sqlite3.c     :  14164 us
  codegen     sqlite3.c     : 114557 us
  link        sqlite3.so    :  15580 us

RCC -O1:
  preprocess  sqlite3.c     : 217979 us
  parse       sqlite3.c     :  59488 us
  typecheck   sqlite3.c     :  12002 us
  opt         sqlite3.c     : 185854 us
  codegen     sqlite3.c     : 140935 us
  link        sqlite3.so    :  13763 us

RCC -O2:
  preprocess  sqlite3.c     : 216731 us
  parse       sqlite3.c     :  58479 us
  typecheck   sqlite3.c     :  13077 us
  opt         sqlite3.c     : 149623 us
  codegen     sqlite3.c     : 119118 us
  link        sqlite3.so    :  17975 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1255 ms |
| RCC -O1   |       807 ms |
| RCC -O2   |       664 ms |
| TCC       |        98 ms |
| GCC -O0   |      1130 ms |
| GCC -O2   |     10486 ms |
| Clang -O0 |      1267 ms |
| Clang -O2 |     11768 ms |
