# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          183 |          886 |       1069 |
| RCC -O1   |          185 |          825 |       1010 |
| RCC -O2   |          113 |          940 |       1053 |
| TCC       |          153 |          754 |        907 |
| GCC -O0   |          149 |          693 |        842 |
| GCC -O2   |          243 |          376 |        619 |
| Clang -O0 |          114 |          626 |        740 |
| Clang -O2 |          219 |          392 |        611 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    920 us
  parse       bench.c       :    215 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    195 us
  link        bench_rcc     :   1226 us
  link        bench_rcc     :  82259 us

RCC -O1:
  preprocess  bench.c       :    716 us
  parse       bench.c       :    173 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     36 us
  codegen     bench.c       :    168 us
  link        bench_o1      :    517 us
  link        bench_o1      : 144381 us

RCC -O2:
  preprocess  bench.c       :   2203 us
  parse       bench.c       :    427 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     50 us
  codegen     bench.c       :    363 us
  link        bench_o2      :    190 us
  link        bench_o2      : 111736 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 552208 us
  parse       sqlite3.c     : 267207 us
  typecheck   sqlite3.c     :  32271 us
  codegen     sqlite3.c     : 274355 us
  link        sqlite3.so    :  25941 us

RCC -O1:
  preprocess  sqlite3.c     : 518979 us
  parse       sqlite3.c     : 105697 us
  typecheck   sqlite3.c     :  18211 us
  opt         sqlite3.c     : 339283 us
  codegen     sqlite3.c     : 260825 us
  link        sqlite3.so    :  23237 us

RCC -O2:
  preprocess  sqlite3.c     : 573269 us
  parse       sqlite3.c     :  83235 us
  typecheck   sqlite3.c     :  38344 us
  opt         sqlite3.c     : 325399 us
  codegen     sqlite3.c     : 296052 us
  link        sqlite3.so    :  18104 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1635 ms |
| RCC -O1   |      1209 ms |
| RCC -O2   |      1236 ms |
| TCC       |       251 ms |
| GCC -O0   |      1945 ms |
| GCC -O2   |     20196 ms |
| Clang -O0 |      1904 ms |
| Clang -O2 |     15954 ms |
