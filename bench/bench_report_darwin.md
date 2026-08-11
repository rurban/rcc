# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           59 |          662 |        721 |
| RCC -O1   |           56 |          621 |        677 |
| RCC -O2   |           70 |          644 |        714 |
| TCC       |           43 |          579 |        622 |
| GCC -O0   |           67 |          446 |        513 |
| GCC -O2   |           99 |          263 |        362 |
| Clang -O0 |           56 |          441 |        497 |
| Clang -O2 |          108 |          297 |        405 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    670 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    111 us
  link        bench_rcc     :     94 us
  link        bench_rcc     :  43572 us

RCC -O1:
  preprocess  bench.c       :    594 us
  parse       bench.c       :    170 us
  typecheck   bench.c       :     11 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    126 us
  link        bench_o1      :    116 us
  link        bench_o1      :  43726 us

RCC -O2:
  preprocess  bench.c       :    559 us
  parse       bench.c       :    156 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    127 us
  link        bench_o2      :    329 us
  link        bench_o2      :  45814 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 203183 us
  parse       sqlite3.c     :  42772 us
  typecheck   sqlite3.c     :  11441 us
  codegen     sqlite3.c     :  86552 us
  link        sqlite3.so    :  14740 us

RCC -O1:
  preprocess  sqlite3.c     : 196852 us
  parse       sqlite3.c     :  41319 us
  typecheck   sqlite3.c     :  11377 us
  opt         sqlite3.c     :  18603 us
  codegen     sqlite3.c     :  95398 us
  link        sqlite3.so    :  15596 us

RCC -O2:
  preprocess  sqlite3.c     : 206250 us
  parse       sqlite3.c     :  43632 us
  typecheck   sqlite3.c     :  11363 us
  opt         sqlite3.c     : 137271 us
  codegen     sqlite3.c     : 148022 us
  link        sqlite3.so    :  15232 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       882 ms |
| RCC -O1   |       582 ms |
| RCC -O2   |       670 ms |
| TCC       |       106 ms |
| GCC -O0   |      1031 ms |
| GCC -O2   |     12159 ms |
| Clang -O0 |      1253 ms |
| Clang -O2 |     16276 ms |
