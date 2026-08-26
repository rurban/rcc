# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           49 |          599 |        648 |
| RCC -O1   |           49 |          616 |        665 |
| RCC -O2   |           65 |          629 |        694 |
| TCC       |           65 |          550 |        615 |
| GCC -O0   |           94 |          484 |        578 |
| GCC -O2   |          130 |          270 |        400 |
| Clang -O0 |           52 |          435 |        487 |
| Clang -O2 |          100 |          271 |        371 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    943 us
  parse       bench.c       :    225 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    188 us
  link        bench_rcc     :    169 us
  link        bench_rcc     :  63916 us

RCC -O1:
  preprocess  bench.c       :    748 us
  parse       bench.c       :    160 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    150 us
  link        bench_o1      :    237 us
  link        bench_o1      :  67660 us

RCC -O2:
  preprocess  bench.c       :    633 us
  parse       bench.c       :    141 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    132 us
  link        bench_o2      :    172 us
  link        bench_o2      :  46010 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 285371 us
  parse       sqlite3.c     :  66579 us
  typecheck   sqlite3.c     :  15873 us
  codegen     sqlite3.c     :  88099 us
  link        sqlite3.so    :  15737 us

RCC -O1:
  preprocess  sqlite3.c     : 250946 us
  parse       sqlite3.c     :  61875 us
  typecheck   sqlite3.c     :  19197 us
  opt         sqlite3.c     : 153145 us
  codegen     sqlite3.c     : 114433 us
  link        sqlite3.so    :  18399 us

RCC -O2:
  preprocess  sqlite3.c     : 246772 us
  parse       sqlite3.c     :  47576 us
  typecheck   sqlite3.c     :  11745 us
  opt         sqlite3.c     : 154273 us
  codegen     sqlite3.c     :  89469 us
  link        sqlite3.so    :  14414 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       532 ms |
| RCC -O1   |       826 ms |
| RCC -O2   |       755 ms |
| TCC       |       105 ms |
| GCC -O0   |      1014 ms |
| GCC -O2   |     11048 ms |
| Clang -O0 |      1227 ms |
| Clang -O2 |     11828 ms |
