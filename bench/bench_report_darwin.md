# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           68 |          651 |        719 |
| RCC -O1   |           67 |          652 |        719 |
| RCC -O2   |          109 |          649 |        758 |
| TCC       |           47 |          567 |        614 |
| GCC -O0   |           67 |          471 |        538 |
| GCC -O2   |          117 |          286 |        403 |
| Clang -O0 |           59 |          470 |        529 |
| Clang -O2 |          101 |          301 |        402 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1436 us
  parse       bench.c       :    333 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    241 us
  link        bench_rcc     :    210 us
  link        bench_rcc     :  64174 us

RCC -O1:
  preprocess  bench.c       :    761 us
  parse       bench.c       :    170 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    169 us
  link        bench_o1      :    164 us
  link        bench_o1      :  56944 us

RCC -O2:
  preprocess  bench.c       :    648 us
  parse       bench.c       :    174 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    129 us
  link        bench_o2      :    159 us
  link        bench_o2      :  54812 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 296070 us
  parse       sqlite3.c     :  64643 us
  typecheck   sqlite3.c     :  15892 us
  codegen     sqlite3.c     : 115459 us
  link        sqlite3.so    :  15332 us

RCC -O1:
  preprocess  sqlite3.c     : 251671 us
  parse       sqlite3.c     :  92556 us
  typecheck   sqlite3.c     :  25513 us
  opt         sqlite3.c     : 151528 us
  codegen     sqlite3.c     : 121168 us
  link        sqlite3.so    :  16122 us

RCC -O2:
  preprocess  sqlite3.c     : 226421 us
  parse       sqlite3.c     :  55242 us
  typecheck   sqlite3.c     :  14326 us
  opt         sqlite3.c     : 164274 us
  codegen     sqlite3.c     : 113308 us
  link        sqlite3.so    :  15581 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       711 ms |
| RCC -O1   |       810 ms |
| RCC -O2   |       762 ms |
| TCC       |       107 ms |
| GCC -O0   |      1090 ms |
| GCC -O2   |     11299 ms |
| Clang -O0 |      1043 ms |
| Clang -O2 |     11613 ms |
