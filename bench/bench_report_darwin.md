# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           51 |          583 |        634 |
| RCC -O1   |           48 |          584 |        632 |
| RCC -O2   |           52 |          582 |        634 |
| TCC       |           40 |          512 |        552 |
| GCC -O0   |           58 |          457 |        515 |
| GCC -O2   |          120 |          263 |        383 |
| Clang -O0 |           50 |          433 |        483 |
| Clang -O2 |           78 |          263 |        341 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    803 us
  parse       bench.c       :    183 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    142 us
  link        bench_rcc     :    103 us
  link        bench_rcc     :  50382 us

RCC -O1:
  preprocess  bench.c       :   1231 us
  parse       bench.c       :    289 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    172 us
  link        bench_o1      :    162 us
  link        bench_o1      :  51397 us

RCC -O2:
  preprocess  bench.c       :    670 us
  parse       bench.c       :    167 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    145 us
  link        bench_o2      :    168 us
  link        bench_o2      :  48741 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 261713 us
  parse       sqlite3.c     :  66403 us
  typecheck   sqlite3.c     :  13565 us
  codegen     sqlite3.c     :  87637 us
  link        sqlite3.so    :  13601 us

RCC -O1:
  preprocess  sqlite3.c     : 201313 us
  parse       sqlite3.c     :  42318 us
  typecheck   sqlite3.c     :  12453 us
  opt         sqlite3.c     :  18510 us
  codegen     sqlite3.c     :  89741 us
  link        sqlite3.so    :  14077 us

RCC -O2:
  preprocess  sqlite3.c     : 187013 us
  parse       sqlite3.c     :  40795 us
  typecheck   sqlite3.c     :  11228 us
  opt         sqlite3.c     : 122313 us
  codegen     sqlite3.c     :  84569 us
  link        sqlite3.so    :  14733 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       510 ms |
| RCC -O1   |       529 ms |
| RCC -O2   |       636 ms |
| TCC       |        85 ms |
| GCC -O0   |       933 ms |
| GCC -O2   |      9408 ms |
| Clang -O0 |       954 ms |
| Clang -O2 |      8787 ms |
