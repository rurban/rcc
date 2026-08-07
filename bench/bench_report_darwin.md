# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          155 |          766 |        921 |
| RCC -O1   |          114 |          703 |        817 |
| RCC -O2   |           91 |          649 |        740 |
| TCC       |           67 |          608 |        675 |
| GCC -O0   |          136 |          632 |        768 |
| GCC -O2   |          193 |          340 |        533 |
| Clang -O0 |           96 |          581 |        677 |
| Clang -O2 |          150 |          332 |        482 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    887 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    146 us
  link        bench_rcc     :    121 us
  link        bench_rcc     : 104773 us

RCC -O1:
  preprocess  bench.c       :   1612 us
  parse       bench.c       :    140 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    120 us
  link        bench_o1      :    194 us
  link        bench_o1      :  70880 us

RCC -O2:
  preprocess  bench.c       :    643 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    120 us
  link        bench_o2      :    221 us
  link        bench_o2      :  71155 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 472066 us
  parse       sqlite3.c     : 205984 us
  typecheck   sqlite3.c     :  31231 us
  codegen     sqlite3.c     : 167255 us
  link        sqlite3.so    :  17471 us

RCC -O1:
  preprocess  sqlite3.c     : 512141 us
  parse       sqlite3.c     :  91326 us
  typecheck   sqlite3.c     :  20159 us
  opt         sqlite3.c     :  39215 us
  codegen     sqlite3.c     : 162400 us
  link        sqlite3.so    :  18714 us

RCC -O2:
  preprocess  sqlite3.c     : 409652 us
  parse       sqlite3.c     :  84061 us
  typecheck   sqlite3.c     :  14364 us
  opt         sqlite3.c     : 214006 us
  codegen     sqlite3.c     : 167219 us
  link        sqlite3.so    :  32255 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1095 ms |
| RCC -O1   |       988 ms |
| RCC -O2   |       993 ms |
| TCC       |       169 ms |
| GCC -O0   |      1522 ms |
| GCC -O2   |     15541 ms |
| Clang -O0 |      1637 ms |
| Clang -O2 |     16409 ms |
