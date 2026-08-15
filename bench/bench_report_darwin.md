# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           66 |          644 |        710 |
| RCC -O1   |           55 |          637 |        692 |
| RCC -O2   |           67 |          649 |        716 |
| TCC       |           43 |          554 |        597 |
| GCC -O0   |           73 |          468 |        541 |
| GCC -O2   |          101 |          283 |        384 |
| Clang -O0 |           56 |          468 |        524 |
| Clang -O2 |           87 |          284 |        371 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    664 us
  parse       bench.c       :    124 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    125 us
  link        bench_rcc     :     82 us
  link        bench_rcc     :  47397 us

RCC -O1:
  preprocess  bench.c       :    603 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    121 us
  link        bench_o1      :    325 us
  link        bench_o1      :  46464 us

RCC -O2:
  preprocess  bench.c       :    613 us
  parse       bench.c       :    128 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    122 us
  link        bench_o2      :    140 us
  link        bench_o2      :  47225 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 233288 us
  parse       sqlite3.c     :  49169 us
  typecheck   sqlite3.c     :  13510 us
  codegen     sqlite3.c     :  96272 us
  link        sqlite3.so    :  14299 us

RCC -O1:
  preprocess  sqlite3.c     : 213987 us
  parse       sqlite3.c     :  49731 us
  typecheck   sqlite3.c     :  13056 us
  opt         sqlite3.c     : 132542 us
  codegen     sqlite3.c     :  97125 us
  link        sqlite3.so    :  15399 us

RCC -O2:
  preprocess  sqlite3.c     : 204774 us
  parse       sqlite3.c     :  46368 us
  typecheck   sqlite3.c     :  13295 us
  opt         sqlite3.c     : 137480 us
  codegen     sqlite3.c     :  94260 us
  link        sqlite3.so    :  15203 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       655 ms |
| RCC -O1   |       771 ms |
| RCC -O2   |       742 ms |
| TCC       |       103 ms |
| GCC -O0   |      1017 ms |
| GCC -O2   |      9734 ms |
| Clang -O0 |      1064 ms |
| Clang -O2 |     10171 ms |
