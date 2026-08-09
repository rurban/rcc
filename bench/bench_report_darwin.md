# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          106 |          699 |        805 |
| RCC -O1   |           77 |          717 |        794 |
| RCC -O2   |           84 |          687 |        771 |
| TCC       |           76 |          632 |        708 |
| GCC -O0   |           97 |          519 |        616 |
| GCC -O2   |          149 |          321 |        470 |
| Clang -O0 |           81 |          529 |        610 |
| Clang -O2 |          130 |          321 |        451 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    974 us
  parse       bench.c       :    251 us
  typecheck   bench.c       :     16 us
  codegen     bench.c       :    312 us
  link        bench_rcc     :    466 us
  link        bench_rcc     :  67372 us

RCC -O1:
  preprocess  bench.c       :    747 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    164 us
  link        bench_o1      :    106 us
  link        bench_o1      :  64672 us

RCC -O2:
  preprocess  bench.c       :   1672 us
  parse       bench.c       :    169 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    373 us
  link        bench_o2      :    440 us
  link        bench_o2      :  72362 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 468360 us
  parse       sqlite3.c     :  68525 us
  typecheck   sqlite3.c     :  22669 us
  codegen     sqlite3.c     : 129728 us
  link        sqlite3.so    :  20392 us

RCC -O1:
  preprocess  sqlite3.c     : 291845 us
  parse       sqlite3.c     :  59141 us
  typecheck   sqlite3.c     :  16517 us
  opt         sqlite3.c     :  24858 us
  codegen     sqlite3.c     : 131087 us
  link        sqlite3.so    :  24689 us

RCC -O2:
  preprocess  sqlite3.c     : 321950 us
  parse       sqlite3.c     :  71189 us
  typecheck   sqlite3.c     :  15147 us
  opt         sqlite3.c     : 182362 us
  codegen     sqlite3.c     : 129165 us
  link        sqlite3.so    :  17506 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       788 ms |
| RCC -O1   |       802 ms |
| RCC -O2   |       925 ms |
| TCC       |       127 ms |
| GCC -O0   |      1254 ms |
| GCC -O2   |     13669 ms |
| Clang -O0 |      1573 ms |
| Clang -O2 |     14885 ms |
