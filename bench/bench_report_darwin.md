# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          129 |          902 |       1031 |
| RCC -O1   |          125 |          813 |        938 |
| RCC -O2   |           98 |          869 |        967 |
| TCC       |          105 |          764 |        869 |
| GCC -O0   |          175 |          661 |        836 |
| GCC -O2   |          160 |          409 |        569 |
| Clang -O0 |          166 |          737 |        903 |
| Clang -O2 |          303 |          392 |        695 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2369 us
  parse       bench.c       :    352 us
  typecheck   bench.c       :      9 us
  codegen     bench.c       :    342 us
  link        bench_rcc     :    277 us
  link        bench_rcc     :  89467 us

RCC -O1:
  preprocess  bench.c       :    788 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    150 us
  link        bench_o1      :    191 us
  link        bench_o1      : 106713 us

RCC -O2:
  preprocess  bench.c       :    675 us
  parse       bench.c       :    136 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    126 us
  link        bench_o2      :    199 us
  link        bench_o2      :  69781 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 420715 us
  parse       sqlite3.c     : 161612 us
  typecheck   sqlite3.c     :  37649 us
  codegen     sqlite3.c     : 215070 us
  link        sqlite3.so    :  20704 us

RCC -O1:
  preprocess  sqlite3.c     : 434699 us
  parse       sqlite3.c     :  72375 us
  typecheck   sqlite3.c     :  20253 us
  opt         sqlite3.c     : 269326 us
  codegen     sqlite3.c     : 224123 us
  link        sqlite3.so    :  32416 us

RCC -O2:
  preprocess  sqlite3.c     : 394359 us
  parse       sqlite3.c     :  80439 us
  typecheck   sqlite3.c     :  30249 us
  opt         sqlite3.c     : 345368 us
  codegen     sqlite3.c     : 172205 us
  link        sqlite3.so    :  21024 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1753 ms |
| RCC -O1   |      1720 ms |
| RCC -O2   |      2167 ms |
| TCC       |       228 ms |
| GCC -O0   |      2417 ms |
| GCC -O2   |     21399 ms |
| Clang -O0 |      2097 ms |
| Clang -O2 |     23213 ms |
