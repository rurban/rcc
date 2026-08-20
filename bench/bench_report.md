# Linux RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           16 |          717 |        733 |
| RCC -O1   |           17 |          704 |        721 |
| RCC -O2   |           16 |          710 |        726 |
| TCC       |           14 |          489 |        503 |
| SLIMCC    |           59 |          509 |        568 |
| XCC       |           41 |          362 |        403 |
| KEFIR     |          276 |          583 |        859 |
| KEFIR -O1 |          213 |          313 |        526 |
| SCC       |          132 |          544 |        676 |
| LACC      |           38 |          761 |        799 |
| ANTCC     |           31 |          420 |        451 |
| CCC       |           36 |          554 |        590 |
| GCC -O0   |           66 |          489 |        555 |
| GCC -O2   |          186 |          180 |        366 |
| Clang -O0 |          443 |          470 |        913 |
| Clang -O2 |          228 |          182 |        410 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   9190 us
  parse       bench.c       :    717 us
  typecheck   bench.c       :      7 us
  codegen     bench.c       :    329 us
  link        bench_rcc     :    669 us

RCC -O1:
  preprocess  bench.c       :   8954 us
  parse       bench.c       :    546 us
  typecheck   bench.c       :     28 us
  opt         bench.c       :     60 us
  codegen     bench.c       :    402 us
  link        bench_o1      :    614 us

RCC -O2:
  preprocess  bench.c       :   9225 us
  parse       bench.c       :    585 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    331 us
  link        bench_o2      :    553 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 246734 us
  parse       sqlite3.c     : 134542 us
  typecheck   sqlite3.c     :   9851 us
  codegen     sqlite3.c     : 196088 us
  link        sqlite3.so    :   6426 us

RCC -O1:
  preprocess  sqlite3.c     : 243341 us
  parse       sqlite3.c     : 134619 us
  typecheck   sqlite3.c     :   9675 us
  opt         sqlite3.c     : 216150 us
  codegen     sqlite3.c     : 192066 us
  link        sqlite3.so    :   6615 us

RCC -O2:
  preprocess  sqlite3.c     : 246654 us
  parse       sqlite3.c     : 134188 us
  typecheck   sqlite3.c     :  10019 us
  opt         sqlite3.c     : 224599 us
  codegen     sqlite3.c     : 193115 us
  link        sqlite3.so    :   6503 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       875 ms |
| RCC -O1   |      1090 ms |
| RCC -O2   |      1092 ms |
| TCC       |       113 ms |
| SLIMCC    |       669 ms |
| KEFIR     |     18346 ms |
| KEFIR -O1 |     33144 ms |
| ANTCC     |       392 ms |
| CCC       |     12304 ms |
| GCC -O0   |      4096 ms |
| GCC -O2   |     25883 ms |
| Clang -O0 |      1854 ms |
| Clang -O2 |     20209 ms |
