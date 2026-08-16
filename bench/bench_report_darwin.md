# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          110 |          714 |        824 |
| RCC -O1   |           70 |          682 |        752 |
| RCC -O2   |           64 |          686 |        750 |
| TCC       |           65 |          653 |        718 |
| GCC -O0   |          129 |          495 |        624 |
| GCC -O2   |          136 |          298 |        434 |
| Clang -O0 |           65 |          505 |        570 |
| Clang -O2 |           99 |          298 |        397 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    936 us
  parse       bench.c       :    223 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    177 us
  link        bench_rcc     :     77 us
  link        bench_rcc     :  57862 us

RCC -O1:
  preprocess  bench.c       :    681 us
  parse       bench.c       :    202 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    160 us
  link        bench_o1      :    127 us
  link        bench_o1      :  58199 us

RCC -O2:
  preprocess  bench.c       :    734 us
  parse       bench.c       :    192 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    167 us
  link        bench_o2      :     79 us
  link        bench_o2      :  57927 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 295577 us
  parse       sqlite3.c     :  62666 us
  typecheck   sqlite3.c     :  16600 us
  codegen     sqlite3.c     : 122266 us
  link        sqlite3.so    :  17802 us

RCC -O1:
  preprocess  sqlite3.c     : 313598 us
  parse       sqlite3.c     :  83808 us
  typecheck   sqlite3.c     :  19569 us
  opt         sqlite3.c     : 276607 us
  codegen     sqlite3.c     : 131758 us
  link        sqlite3.so    :  15106 us

RCC -O2:
  preprocess  sqlite3.c     : 249121 us
  parse       sqlite3.c     :  53952 us
  typecheck   sqlite3.c     :  12383 us
  opt         sqlite3.c     : 174949 us
  codegen     sqlite3.c     : 269370 us
  link        sqlite3.so    :  16749 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1052 ms |
| RCC -O1   |       916 ms |
| RCC -O2   |      1213 ms |
| TCC       |       114 ms |
| GCC -O0   |      1588 ms |
| GCC -O2   |     13040 ms |
| Clang -O0 |      1847 ms |
| Clang -O2 |     13623 ms |
