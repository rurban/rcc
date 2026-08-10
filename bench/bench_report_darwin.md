# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          105 |          735 |        840 |
| RCC -O1   |           95 |          731 |        826 |
| RCC -O2   |           92 |          720 |        812 |
| TCC       |           80 |          642 |        722 |
| GCC -O0   |          126 |          550 |        676 |
| GCC -O2   |          135 |          333 |        468 |
| Clang -O0 |          105 |          642 |        747 |
| Clang -O2 |          203 |          365 |        568 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    960 us
  parse       bench.c       :    191 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    166 us
  link        bench_rcc     :    438 us
  link        bench_rcc     :  91362 us

RCC -O1:
  preprocess  bench.c       :    902 us
  parse       bench.c       :    305 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    191 us
  link        bench_o1      :    180 us
  link        bench_o1      :  91110 us

RCC -O2:
  preprocess  bench.c       :   1248 us
  parse       bench.c       :    311 us
  typecheck   bench.c       :      9 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    131 us
  link        bench_o2      :     79 us
  link        bench_o2      : 130604 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 459401 us
  parse       sqlite3.c     : 182169 us
  typecheck   sqlite3.c     :  30173 us
  codegen     sqlite3.c     : 160308 us
  link        sqlite3.so    :  21375 us

RCC -O1:
  preprocess  sqlite3.c     : 321781 us
  parse       sqlite3.c     :  60501 us
  typecheck   sqlite3.c     :  19727 us
  opt         sqlite3.c     :  31990 us
  codegen     sqlite3.c     : 148663 us
  link        sqlite3.so    :  21256 us

RCC -O2:
  preprocess  sqlite3.c     : 503212 us
  parse       sqlite3.c     :  99244 us
  typecheck   sqlite3.c     :  25978 us
  opt         sqlite3.c     : 434470 us
  codegen     sqlite3.c     : 236864 us
  link        sqlite3.so    :  21803 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1258 ms |
| RCC -O1   |       981 ms |
| RCC -O2   |      1233 ms |
| TCC       |       167 ms |
| GCC -O0   |      1728 ms |
| GCC -O2   |     13304 ms |
| Clang -O0 |      1236 ms |
| Clang -O2 |     11821 ms |
