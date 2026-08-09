# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          121 |          888 |       1009 |
| RCC -O1   |          107 |          854 |        961 |
| RCC -O2   |          125 |          789 |        914 |
| TCC       |           86 |          717 |        803 |
| GCC -O0   |          154 |          593 |        747 |
| GCC -O2   |          159 |          349 |        508 |
| Clang -O0 |          295 |          622 |        917 |
| Clang -O2 |          248 |          343 |        591 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1091 us
  parse       bench.c       :    209 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    163 us
  link        bench_rcc     :    421 us
  link        bench_rcc     :  91148 us

RCC -O1:
  preprocess  bench.c       :    671 us
  parse       bench.c       :    161 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    124 us
  link        bench_o1      :    226 us
  link        bench_o1      :  69762 us

RCC -O2:
  preprocess  bench.c       :    714 us
  parse       bench.c       :    217 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    143 us
  link        bench_o2      :    235 us
  link        bench_o2      :  74058 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 408149 us
  parse       sqlite3.c     : 262621 us
  typecheck   sqlite3.c     :  24542 us
  codegen     sqlite3.c     : 318628 us
  link        sqlite3.so    :  88994 us

RCC -O1:
  preprocess  sqlite3.c     : 414978 us
  parse       sqlite3.c     : 120221 us
  typecheck   sqlite3.c     :  42437 us
  opt         sqlite3.c     :  38136 us
  codegen     sqlite3.c     : 230659 us
  link        sqlite3.so    :  19400 us

RCC -O2:
  preprocess  sqlite3.c     : 435138 us
  parse       sqlite3.c     :  58755 us
  typecheck   sqlite3.c     :  15994 us
  opt         sqlite3.c     : 301165 us
  codegen     sqlite3.c     : 207165 us
  link        sqlite3.so    :  25350 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1259 ms |
| RCC -O1   |       935 ms |
| RCC -O2   |      1025 ms |
| TCC       |       149 ms |
| GCC -O0   |      1786 ms |
| GCC -O2   |     13453 ms |
| Clang -O0 |      1436 ms |
| Clang -O2 |     14374 ms |
