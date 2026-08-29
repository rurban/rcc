# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           92 |          635 |        727 |
| RCC -O1   |           63 |          601 |        664 |
| RCC -O2   |           54 |          606 |        660 |
| TCC       |           43 |          516 |        559 |
| GCC -O0   |          117 |          483 |        600 |
| GCC -O2   |          117 |          270 |        387 |
| Clang -O0 |           63 |          458 |        521 |
| Clang -O2 |          102 |          283 |        385 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    959 us
  parse       bench.c       :    155 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    194 us
  link        bench_rcc     :    518 us
  link        bench_rcc     :  77365 us

RCC -O1:
  preprocess  bench.c       :    680 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    143 us
  link        bench_o1      :    432 us
  link        bench_o1      :  75912 us

RCC -O2:
  preprocess  bench.c       :    684 us
  parse       bench.c       :    177 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    142 us
  link        bench_o2      :    321 us
  link        bench_o2      :  58477 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 284217 us
  parse       sqlite3.c     : 211121 us
  typecheck   sqlite3.c     :  23170 us
  codegen     sqlite3.c     : 151081 us
  link        sqlite3.so    :  17740 us

RCC -O1:
  preprocess  sqlite3.c     : 320619 us
  parse       sqlite3.c     :  77609 us
  typecheck   sqlite3.c     :  24356 us
  opt         sqlite3.c     : 272135 us
  codegen     sqlite3.c     : 181229 us
  link        sqlite3.so    :  23106 us

RCC -O2:
  preprocess  sqlite3.c     : 226676 us
  parse       sqlite3.c     :  56226 us
  typecheck   sqlite3.c     :  12980 us
  opt         sqlite3.c     : 154295 us
  codegen     sqlite3.c     : 128918 us
  link        sqlite3.so    :  15641 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1050 ms |
| RCC -O1   |       826 ms |
| RCC -O2   |       851 ms |
| TCC       |       113 ms |
| GCC -O0   |      1232 ms |
| GCC -O2   |     10891 ms |
| Clang -O0 |      1061 ms |
| Clang -O2 |      8924 ms |
