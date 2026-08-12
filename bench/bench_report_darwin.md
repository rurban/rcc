# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           58 |          643 |        701 |
| RCC -O1   |           54 |          645 |        699 |
| RCC -O2   |           86 |          665 |        751 |
| TCC       |           67 |          631 |        698 |
| GCC -O0   |           92 |          501 |        593 |
| GCC -O2   |          160 |          309 |        469 |
| Clang -O0 |           71 |          498 |        569 |
| Clang -O2 |          115 |          303 |        418 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    829 us
  parse       bench.c       :    172 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    143 us
  link        bench_rcc     :     96 us
  link        bench_rcc     :  59074 us

RCC -O1:
  preprocess  bench.c       :    706 us
  parse       bench.c       :    181 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    158 us
  link        bench_o1      :    110 us
  link        bench_o1      :  57795 us

RCC -O2:
  preprocess  bench.c       :    764 us
  parse       bench.c       :    176 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    155 us
  link        bench_o2      :    134 us
  link        bench_o2      :  55894 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 304195 us
  parse       sqlite3.c     :  57401 us
  typecheck   sqlite3.c     :  13359 us
  codegen     sqlite3.c     : 115786 us
  link        sqlite3.so    :  16204 us

RCC -O1:
  preprocess  sqlite3.c     : 258008 us
  parse       sqlite3.c     :  54331 us
  typecheck   sqlite3.c     :  15311 us
  opt         sqlite3.c     :  23573 us
  codegen     sqlite3.c     : 101066 us
  link        sqlite3.so    :  15473 us

RCC -O2:
  preprocess  sqlite3.c     : 223106 us
  parse       sqlite3.c     :  47578 us
  typecheck   sqlite3.c     :  13760 us
  opt         sqlite3.c     : 136668 us
  codegen     sqlite3.c     : 107858 us
  link        sqlite3.so    :  17677 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       706 ms |
| RCC -O1   |       669 ms |
| RCC -O2   |       808 ms |
| TCC       |       105 ms |
| GCC -O0   |      1417 ms |
| GCC -O2   |     14136 ms |
| Clang -O0 |      1565 ms |
| Clang -O2 |     13611 ms |
