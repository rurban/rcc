# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          111 |          839 |        950 |
| RCC -O1   |          104 |          814 |        918 |
| RCC -O2   |           99 |          787 |        886 |
| TCC       |           84 |          691 |        775 |
| GCC -O0   |          112 |          583 |        695 |
| GCC -O2   |          158 |          350 |        508 |
| Clang -O0 |          207 |          573 |        780 |
| Clang -O2 |          128 |          337 |        465 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    931 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    137 us
  link        bench_rcc     :    320 us
  link        bench_rcc     :  63192 us

RCC -O1:
  preprocess  bench.c       :    675 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    166 us
  link        bench_o1      :    227 us
  link        bench_o1      :  60079 us

RCC -O2:
  preprocess  bench.c       :   1544 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    138 us
  link        bench_o2      :    441 us
  link        bench_o2      :  69777 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 325136 us
  parse       sqlite3.c     : 189544 us
  typecheck   sqlite3.c     :  29722 us
  codegen     sqlite3.c     : 155767 us
  link        sqlite3.so    :  17636 us

RCC -O1:
  preprocess  sqlite3.c     : 296747 us
  parse       sqlite3.c     :  93016 us
  typecheck   sqlite3.c     :  19214 us
  opt         sqlite3.c     : 262196 us
  codegen     sqlite3.c     : 151078 us
  link        sqlite3.so    :  53093 us

RCC -O2:
  preprocess  sqlite3.c     : 307506 us
  parse       sqlite3.c     :  75092 us
  typecheck   sqlite3.c     :  16857 us
  opt         sqlite3.c     : 270784 us
  codegen     sqlite3.c     : 165370 us
  link        sqlite3.so    :  24981 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1343 ms |
| RCC -O1   |      1143 ms |
| RCC -O2   |      1206 ms |
| TCC       |       126 ms |
| GCC -O0   |      1605 ms |
| GCC -O2   |     17094 ms |
| Clang -O0 |      2358 ms |
| Clang -O2 |     14323 ms |
