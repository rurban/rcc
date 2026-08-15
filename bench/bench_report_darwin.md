# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          109 |          687 |        796 |
| RCC -O1   |           58 |          612 |        670 |
| RCC -O2   |           48 |          596 |        644 |
| TCC       |           39 |          517 |        556 |
| GCC -O0   |           59 |          434 |        493 |
| GCC -O2   |           90 |          263 |        353 |
| Clang -O0 |           58 |          451 |        509 |
| Clang -O2 |           84 |          271 |        355 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    923 us
  parse       bench.c       :    161 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    121 us
  link        bench_rcc     :     67 us
  link        bench_rcc     :  45140 us

RCC -O1:
  preprocess  bench.c       :    610 us
  parse       bench.c       :    175 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    140 us
  link        bench_o1      :    114 us
  link        bench_o1      :  44578 us

RCC -O2:
  preprocess  bench.c       :    679 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    125 us
  link        bench_o2      :     65 us
  link        bench_o2      :  51128 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 249590 us
  parse       sqlite3.c     :  55913 us
  typecheck   sqlite3.c     :  16489 us
  codegen     sqlite3.c     : 132610 us
  link        sqlite3.so    :  15382 us

RCC -O1:
  preprocess  sqlite3.c     : 255649 us
  parse       sqlite3.c     :  50431 us
  typecheck   sqlite3.c     :  13118 us
  opt         sqlite3.c     : 149294 us
  codegen     sqlite3.c     : 101542 us
  link        sqlite3.so    :  14337 us

RCC -O2:
  preprocess  sqlite3.c     : 267705 us
  parse       sqlite3.c     :  55388 us
  typecheck   sqlite3.c     :  22870 us
  opt         sqlite3.c     : 161459 us
  codegen     sqlite3.c     : 137084 us
  link        sqlite3.so    :  22342 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       739 ms |
| RCC -O1   |       706 ms |
| RCC -O2   |       679 ms |
| TCC       |       106 ms |
| GCC -O0   |      1169 ms |
| GCC -O2   |      9166 ms |
| Clang -O0 |      1220 ms |
| Clang -O2 |      8665 ms |
