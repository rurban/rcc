# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           73 |          699 |        772 |
| RCC -O1   |          117 |          671 |        788 |
| RCC -O2   |           84 |          664 |        748 |
| TCC       |           72 |          609 |        681 |
| GCC -O0   |          124 |          513 |        637 |
| GCC -O2   |          137 |          307 |        444 |
| Clang -O0 |          107 |          492 |        599 |
| Clang -O2 |          107 |          291 |        398 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    840 us
  parse       bench.c       :    236 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    215 us
  link        bench_rcc     :    209 us
  link        bench_rcc     :  63254 us

RCC -O1:
  preprocess  bench.c       :    748 us
  parse       bench.c       :    199 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    140 us
  link        bench_o1      :    120 us
  link        bench_o1      :  64658 us

RCC -O2:
  preprocess  bench.c       :    761 us
  parse       bench.c       :    263 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    173 us
  link        bench_o2      :    151 us
  link        bench_o2      :  75979 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 302162 us
  parse       sqlite3.c     :  64960 us
  typecheck   sqlite3.c     :  12232 us
  codegen     sqlite3.c     : 109077 us
  link        sqlite3.so    :  17995 us

RCC -O1:
  preprocess  sqlite3.c     : 224936 us
  parse       sqlite3.c     :  53594 us
  typecheck   sqlite3.c     :  11292 us
  opt         sqlite3.c     : 142132 us
  codegen     sqlite3.c     : 102015 us
  link        sqlite3.so    :  16988 us

RCC -O2:
  preprocess  sqlite3.c     : 224682 us
  parse       sqlite3.c     :  76714 us
  typecheck   sqlite3.c     :  15270 us
  opt         sqlite3.c     : 182406 us
  codegen     sqlite3.c     : 118282 us
  link        sqlite3.so    :  19956 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       834 ms |
| RCC -O1   |       856 ms |
| RCC -O2   |       899 ms |
| TCC       |       115 ms |
| GCC -O0   |      1211 ms |
| GCC -O2   |     12060 ms |
| Clang -O0 |      1130 ms |
| Clang -O2 |     10713 ms |
