# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           75 |          661 |        736 |
| RCC -O1   |           61 |          641 |        702 |
| RCC -O2   |           58 |          653 |        711 |
| TCC       |           46 |          578 |        624 |
| GCC -O0   |          101 |          484 |        585 |
| GCC -O2   |          119 |          289 |        408 |
| Clang -O0 |           53 |          467 |        520 |
| Clang -O2 |          100 |          288 |        388 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    726 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    131 us
  link        bench_rcc     :     83 us
  link        bench_rcc     :  67964 us

RCC -O1:
  preprocess  bench.c       :    583 us
  parse       bench.c       :    132 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    112 us
  link        bench_o1      :     97 us
  link        bench_o1      :  49508 us

RCC -O2:
  preprocess  bench.c       :    748 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    156 us
  link        bench_o2      :     66 us
  link        bench_o2      :  61354 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 230830 us
  parse       sqlite3.c     :  72446 us
  typecheck   sqlite3.c     :  26247 us
  codegen     sqlite3.c     : 449100 us
  link        sqlite3.so    :  28393 us

RCC -O1:
  preprocess  sqlite3.c     : 236774 us
  parse       sqlite3.c     :  50814 us
  typecheck   sqlite3.c     :  15277 us
  opt         sqlite3.c     : 164935 us
  codegen     sqlite3.c     : 130453 us
  link        sqlite3.so    :  15049 us

RCC -O2:
  preprocess  sqlite3.c     : 269900 us
  parse       sqlite3.c     :  66634 us
  typecheck   sqlite3.c     :  14553 us
  opt         sqlite3.c     : 154219 us
  codegen     sqlite3.c     : 121256 us
  link        sqlite3.so    :  15900 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       806 ms |
| RCC -O1   |       726 ms |
| RCC -O2   |      1135 ms |
| TCC       |       109 ms |
| GCC -O0   |       966 ms |
| GCC -O2   |      9245 ms |
| Clang -O0 |       914 ms |
| Clang -O2 |      8838 ms |
