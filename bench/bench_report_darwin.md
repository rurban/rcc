# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          130 |          801 |        931 |
| RCC -O1   |          100 |          775 |        875 |
| RCC -O2   |          106 |          746 |        852 |
| TCC       |           45 |          572 |        617 |
| GCC -O0   |          102 |          519 |        621 |
| GCC -O2   |          155 |          346 |        501 |
| Clang -O0 |          105 |          530 |        635 |
| Clang -O2 |          113 |          312 |        425 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1684 us
  parse       bench.c       :    205 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    192 us
  link        bench_rcc     :    220 us
  link        bench_rcc     :  79424 us

RCC -O1:
  preprocess  bench.c       :    709 us
  parse       bench.c       :    167 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     33 us
  codegen     bench.c       :    149 us
  link        bench_o1      :    179 us
  link        bench_o1      :  73527 us

RCC -O2:
  preprocess  bench.c       :    797 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     36 us
  codegen     bench.c       :    145 us
  link        bench_o2      :    406 us
  link        bench_o2      :  80205 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 408963 us
  parse       sqlite3.c     : 127682 us
  typecheck   sqlite3.c     :  20321 us
  codegen     sqlite3.c     : 253586 us
  link        sqlite3.so    :  29242 us

RCC -O1:
  preprocess  sqlite3.c     : 367690 us
  parse       sqlite3.c     : 222567 us
  typecheck   sqlite3.c     :  25951 us
  opt         sqlite3.c     : 312066 us
  codegen     sqlite3.c     : 167386 us
  link        sqlite3.so    :  71123 us

RCC -O2:
  preprocess  sqlite3.c     : 377354 us
  parse       sqlite3.c     :  76715 us
  typecheck   sqlite3.c     :  22200 us
  opt         sqlite3.c     : 240948 us
  codegen     sqlite3.c     : 225038 us
  link        sqlite3.so    :  22793 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1022 ms |
| RCC -O1   |      1131 ms |
| RCC -O2   |      1169 ms |
| TCC       |       358 ms |
| GCC -O0   |      1654 ms |
| GCC -O2   |     14552 ms |
| Clang -O0 |      1505 ms |
| Clang -O2 |     13983 ms |
