# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           62 |          655 |        717 |
| RCC -O1   |           66 |          650 |        716 |
| RCC -O2   |           78 |          647 |        725 |
| TCC       |           67 |          562 |        629 |
| GCC -O0   |           71 |          474 |        545 |
| GCC -O2   |          115 |          289 |        404 |
| Clang -O0 |           72 |          504 |        576 |
| Clang -O2 |          130 |          304 |        434 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1163 us
  parse       bench.c       :    213 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    195 us
  link        bench_rcc     :    495 us
  link        bench_rcc     :  62626 us

RCC -O1:
  preprocess  bench.c       :    711 us
  parse       bench.c       :    187 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    143 us
  link        bench_o1      :    163 us
  link        bench_o1      :  60200 us

RCC -O2:
  preprocess  bench.c       :    729 us
  parse       bench.c       :    197 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    148 us
  link        bench_o2      :    139 us
  link        bench_o2      :  57622 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 293841 us
  parse       sqlite3.c     :  72251 us
  typecheck   sqlite3.c     :  14378 us
  codegen     sqlite3.c     : 112318 us
  link        sqlite3.so    :  15943 us

RCC -O1:
  preprocess  sqlite3.c     : 230160 us
  parse       sqlite3.c     :  52425 us
  typecheck   sqlite3.c     :  13424 us
  opt         sqlite3.c     : 144468 us
  codegen     sqlite3.c     : 101241 us
  link        sqlite3.so    :  16578 us

RCC -O2:
  preprocess  sqlite3.c     : 214309 us
  parse       sqlite3.c     :  52654 us
  typecheck   sqlite3.c     :  12762 us
  opt         sqlite3.c     : 143821 us
  codegen     sqlite3.c     : 104798 us
  link        sqlite3.so    :  16568 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       655 ms |
| RCC -O1   |       751 ms |
| RCC -O2   |       740 ms |
| TCC       |       104 ms |
| GCC -O0   |      1080 ms |
| GCC -O2   |     10608 ms |
| Clang -O0 |      1039 ms |
| Clang -O2 |     11623 ms |
