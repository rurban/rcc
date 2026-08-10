# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           83 |          688 |        771 |
| RCC -O1   |           93 |          680 |        773 |
| RCC -O2   |           65 |          671 |        736 |
| TCC       |           53 |          590 |        643 |
| GCC -O0   |          108 |          536 |        644 |
| GCC -O2   |          166 |          317 |        483 |
| Clang -O0 |           63 |          576 |        639 |
| Clang -O2 |          145 |          329 |        474 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    861 us
  parse       bench.c       :    155 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    160 us
  link        bench_rcc     :    231 us
  link        bench_rcc     :  62493 us

RCC -O1:
  preprocess  bench.c       :    903 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     53 us
  codegen     bench.c       :    137 us
  link        bench_o1      :    209 us
  link        bench_o1      :  75098 us

RCC -O2:
  preprocess  bench.c       :    760 us
  parse       bench.c       :    189 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    142 us
  link        bench_o2      :    128 us
  link        bench_o2      :  51217 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 356953 us
  parse       sqlite3.c     : 202507 us
  typecheck   sqlite3.c     :  25548 us
  codegen     sqlite3.c     : 189546 us
  link        sqlite3.so    :  21186 us

RCC -O1:
  preprocess  sqlite3.c     : 375315 us
  parse       sqlite3.c     :  69443 us
  typecheck   sqlite3.c     :  37653 us
  opt         sqlite3.c     :  22695 us
  codegen     sqlite3.c     : 119346 us
  link        sqlite3.so    :  16779 us

RCC -O2:
  preprocess  sqlite3.c     : 274514 us
  parse       sqlite3.c     :  56846 us
  typecheck   sqlite3.c     :  16551 us
  opt         sqlite3.c     : 150961 us
  codegen     sqlite3.c     : 118480 us
  link        sqlite3.so    :  21952 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1023 ms |
| RCC -O1   |       772 ms |
| RCC -O2   |       936 ms |
| TCC       |       119 ms |
| GCC -O0   |      1140 ms |
| GCC -O2   |     10708 ms |
| Clang -O0 |      1175 ms |
| Clang -O2 |     10620 ms |
