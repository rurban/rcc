# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          128 |          851 |        979 |
| RCC -O1   |          105 |          858 |        963 |
| RCC -O2   |           98 |          850 |        948 |
| TCC       |           70 |          774 |        844 |
| GCC -O0   |          145 |          673 |        818 |
| GCC -O2   |          198 |          361 |        559 |
| Clang -O0 |          126 |          741 |        867 |
| Clang -O2 |          174 |          377 |        551 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1905 us
  parse       bench.c       :    351 us
  typecheck   bench.c       :     10 us
  codegen     bench.c       :    334 us
  link        bench_rcc     :    284 us
  link        bench_rcc     :  89025 us

RCC -O1:
  preprocess  bench.c       :    883 us
  parse       bench.c       :    167 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     29 us
  codegen     bench.c       :    159 us
  link        bench_o1      :    186 us
  link        bench_o1      :  80270 us

RCC -O2:
  preprocess  bench.c       :    746 us
  parse       bench.c       :    144 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    147 us
  link        bench_o2      :    196 us
  link        bench_o2      :  71293 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 363909 us
  parse       sqlite3.c     : 164370 us
  typecheck   sqlite3.c     :  19556 us
  codegen     sqlite3.c     : 192674 us
  link        sqlite3.so    :  18811 us

RCC -O1:
  preprocess  sqlite3.c     : 350771 us
  parse       sqlite3.c     :  78209 us
  typecheck   sqlite3.c     :  86978 us
  opt         sqlite3.c     : 316417 us
  codegen     sqlite3.c     : 164676 us
  link        sqlite3.so    :  21106 us

RCC -O2:
  preprocess  sqlite3.c     : 395016 us
  parse       sqlite3.c     : 137596 us
  typecheck   sqlite3.c     :  34543 us
  opt         sqlite3.c     : 322907 us
  codegen     sqlite3.c     : 275183 us
  link        sqlite3.so    :  28956 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1623 ms |
| RCC -O1   |      1359 ms |
| RCC -O2   |      1367 ms |
| TCC       |       165 ms |
| GCC -O0   |      1953 ms |
| GCC -O2   |     17300 ms |
| Clang -O0 |      1811 ms |
| Clang -O2 |     17257 ms |
