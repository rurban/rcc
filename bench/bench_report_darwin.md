# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          108 |          698 |        806 |
| RCC -O1   |           86 |          770 |        856 |
| RCC -O2   |           90 |          783 |        873 |
| TCC       |           75 |          652 |        727 |
| GCC -O0   |          181 |          582 |        763 |
| GCC -O2   |          151 |          319 |        470 |
| Clang -O0 |           73 |          635 |        708 |
| Clang -O2 |          305 |          351 |        656 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1596 us
  parse       bench.c       :    197 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    197 us
  link        bench_rcc     :    534 us
  link        bench_rcc     :  91095 us

RCC -O1:
  preprocess  bench.c       :    672 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    144 us
  link        bench_o1      :    552 us
  link        bench_o1      :  65947 us

RCC -O2:
  preprocess  bench.c       :    697 us
  parse       bench.c       :    163 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    144 us
  link        bench_o2      :    368 us
  link        bench_o2      :  60146 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 286139 us
  parse       sqlite3.c     :  64885 us
  typecheck   sqlite3.c     :  22419 us
  codegen     sqlite3.c     : 148247 us
  link        sqlite3.so    :  23170 us

RCC -O1:
  preprocess  sqlite3.c     : 287026 us
  parse       sqlite3.c     :  62621 us
  typecheck   sqlite3.c     :  17337 us
  opt         sqlite3.c     : 216059 us
  codegen     sqlite3.c     : 145785 us
  link        sqlite3.so    :  20214 us

RCC -O2:
  preprocess  sqlite3.c     : 285050 us
  parse       sqlite3.c     :  77292 us
  typecheck   sqlite3.c     :  14777 us
  opt         sqlite3.c     : 237250 us
  codegen     sqlite3.c     : 164505 us
  link        sqlite3.so    :  22150 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1259 ms |
| RCC -O1   |      1203 ms |
| RCC -O2   |      1109 ms |
| TCC       |       227 ms |
| GCC -O0   |      1648 ms |
| GCC -O2   |     16497 ms |
| Clang -O0 |      1339 ms |
| Clang -O2 |     13831 ms |
