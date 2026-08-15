# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           64 |          661 |        725 |
| RCC -O1   |           79 |          649 |        728 |
| RCC -O2   |           56 |          620 |        676 |
| TCC       |           47 |          541 |        588 |
| GCC -O0   |           62 |          457 |        519 |
| GCC -O2   |          110 |          275 |        385 |
| Clang -O0 |           60 |          459 |        519 |
| Clang -O2 |           83 |          282 |        365 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    689 us
  parse       bench.c       :    176 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    153 us
  link        bench_rcc     :     96 us
  link        bench_rcc     :  46730 us

RCC -O1:
  preprocess  bench.c       :    642 us
  parse       bench.c       :    130 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    121 us
  link        bench_o1      :    343 us
  link        bench_o1      :  45266 us

RCC -O2:
  preprocess  bench.c       :    578 us
  parse       bench.c       :    117 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    114 us
  link        bench_o2      :    152 us
  link        bench_o2      :  46133 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 222971 us
  parse       sqlite3.c     :  67741 us
  typecheck   sqlite3.c     :  16691 us
  codegen     sqlite3.c     : 100731 us
  link        sqlite3.so    :  16022 us

RCC -O1:
  preprocess  sqlite3.c     : 204380 us
  parse       sqlite3.c     :  47235 us
  typecheck   sqlite3.c     :  12091 us
  opt         sqlite3.c     : 141967 us
  codegen     sqlite3.c     :  91517 us
  link        sqlite3.so    :  14289 us

RCC -O2:
  preprocess  sqlite3.c     : 209335 us
  parse       sqlite3.c     :  43791 us
  typecheck   sqlite3.c     :  12740 us
  opt         sqlite3.c     : 128337 us
  codegen     sqlite3.c     : 140873 us
  link        sqlite3.so    :  15424 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       843 ms |
| RCC -O1   |       776 ms |
| RCC -O2   |       695 ms |
| TCC       |        97 ms |
| GCC -O0   |       982 ms |
| GCC -O2   |     10721 ms |
| Clang -O0 |      1367 ms |
| Clang -O2 |     11004 ms |
