# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          128 |          834 |        962 |
| RCC -O1   |           93 |          779 |        872 |
| RCC -O2   |          115 |          795 |        910 |
| TCC       |           90 |          655 |        745 |
| GCC -O0   |          145 |          575 |        720 |
| GCC -O2   |          147 |          334 |        481 |
| Clang -O0 |           82 |          595 |        677 |
| Clang -O2 |          150 |          324 |        474 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    953 us
  parse       bench.c       :    294 us
  typecheck   bench.c       :     23 us
  codegen     bench.c       :    151 us
  link        bench_rcc     :    162 us
  link        bench_rcc     :  53860 us

RCC -O1:
  preprocess  bench.c       :   1088 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    140 us
  link        bench_o1      :    105 us
  link        bench_o1      : 108135 us

RCC -O2:
  preprocess  bench.c       :   1200 us
  parse       bench.c       :    291 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     74 us
  codegen     bench.c       :    353 us
  link        bench_o2      :    661 us
  link        bench_o2      : 100474 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 564671 us
  parse       sqlite3.c     : 290501 us
  typecheck   sqlite3.c     :  25617 us
  codegen     sqlite3.c     : 245851 us
  link        sqlite3.so    :  20488 us

RCC -O1:
  preprocess  sqlite3.c     : 505276 us
  parse       sqlite3.c     :  81359 us
  typecheck   sqlite3.c     :  30002 us
  opt         sqlite3.c     : 314080 us
  codegen     sqlite3.c     : 208929 us
  link        sqlite3.so    :  18670 us

RCC -O2:
  preprocess  sqlite3.c     : 427926 us
  parse       sqlite3.c     :  93250 us
  typecheck   sqlite3.c     :  21888 us
  opt         sqlite3.c     : 332376 us
  codegen     sqlite3.c     : 210571 us
  link        sqlite3.so    :  24809 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1134 ms |
| RCC -O1   |      1157 ms |
| RCC -O2   |      1177 ms |
| TCC       |       168 ms |
| GCC -O0   |      1553 ms |
| GCC -O2   |     16318 ms |
| Clang -O0 |      1702 ms |
| Clang -O2 |     16321 ms |
