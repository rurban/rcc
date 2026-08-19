# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           98 |          685 |        783 |
| RCC -O1   |           63 |          667 |        730 |
| RCC -O2   |           65 |          688 |        753 |
| TCC       |           63 |          576 |        639 |
| GCC -O0   |           82 |          470 |        552 |
| GCC -O2   |          128 |          296 |        424 |
| Clang -O0 |           66 |          461 |        527 |
| Clang -O2 |           86 |          275 |        361 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    813 us
  parse       bench.c       :    163 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    144 us
  link        bench_rcc     :    256 us
  link        bench_rcc     :  51312 us

RCC -O1:
  preprocess  bench.c       :    714 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    137 us
  link        bench_o1      :     83 us
  link        bench_o1      :  49305 us

RCC -O2:
  preprocess  bench.c       :    730 us
  parse       bench.c       :    196 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    147 us
  link        bench_o2      :    110 us
  link        bench_o2      :  51720 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 293366 us
  parse       sqlite3.c     : 148560 us
  typecheck   sqlite3.c     :  28617 us
  codegen     sqlite3.c     : 144360 us
  link        sqlite3.so    :  18119 us

RCC -O1:
  preprocess  sqlite3.c     : 306539 us
  parse       sqlite3.c     :  53833 us
  typecheck   sqlite3.c     :  12511 us
  opt         sqlite3.c     : 160259 us
  codegen     sqlite3.c     : 142222 us
  link        sqlite3.so    :  17930 us

RCC -O2:
  preprocess  sqlite3.c     : 260045 us
  parse       sqlite3.c     :  56348 us
  typecheck   sqlite3.c     :  14666 us
  opt         sqlite3.c     : 220124 us
  codegen     sqlite3.c     : 162702 us
  link        sqlite3.so    :  16045 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       576 ms |
| RCC -O1   |       707 ms |
| RCC -O2   |       730 ms |
| TCC       |        95 ms |
| GCC -O0   |       991 ms |
| GCC -O2   |     12165 ms |
| Clang -O0 |      1307 ms |
| Clang -O2 |     10633 ms |
