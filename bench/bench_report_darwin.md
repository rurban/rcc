# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           95 |          660 |        755 |
| RCC -O1   |           78 |          643 |        721 |
| RCC -O2   |           66 |          648 |        714 |
| TCC       |           47 |          629 |        676 |
| GCC -O0   |          135 |          514 |        649 |
| GCC -O2   |          161 |          333 |        494 |
| Clang -O0 |           72 |          588 |        660 |
| Clang -O2 |          157 |          356 |        513 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    778 us
  parse       bench.c       :    180 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    147 us
  link        bench_rcc     :    115 us
  link        bench_rcc     :  54584 us

RCC -O1:
  preprocess  bench.c       :    831 us
  parse       bench.c       :    163 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    122 us
  link        bench_o1      :    309 us
  link        bench_o1      :  53334 us

RCC -O2:
  preprocess  bench.c       :    724 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    129 us
  link        bench_o2      :    189 us
  link        bench_o2      :  50976 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 262449 us
  parse       sqlite3.c     : 108336 us
  typecheck   sqlite3.c     :  22987 us
  codegen     sqlite3.c     : 114233 us
  link        sqlite3.so    :  16872 us

RCC -O1:
  preprocess  sqlite3.c     : 262836 us
  parse       sqlite3.c     :  99264 us
  typecheck   sqlite3.c     :  21699 us
  opt         sqlite3.c     : 158802 us
  codegen     sqlite3.c     : 134534 us
  link        sqlite3.so    :  15878 us

RCC -O2:
  preprocess  sqlite3.c     : 250174 us
  parse       sqlite3.c     :  58804 us
  typecheck   sqlite3.c     :  14866 us
  opt         sqlite3.c     : 173329 us
  codegen     sqlite3.c     : 115804 us
  link        sqlite3.so    :  15477 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1283 ms |
| RCC -O1   |      1011 ms |
| RCC -O2   |      1059 ms |
| TCC       |       137 ms |
| GCC -O0   |      1330 ms |
| GCC -O2   |     14409 ms |
| Clang -O0 |      2351 ms |
| Clang -O2 |     22484 ms |
