# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           64 |          643 |        707 |
| RCC -O1   |           86 |          668 |        754 |
| RCC -O2   |           63 |          603 |        666 |
| TCC       |           84 |          569 |        653 |
| GCC -O0   |           99 |          517 |        616 |
| GCC -O2   |          119 |          300 |        419 |
| Clang -O0 |           92 |          513 |        605 |
| Clang -O2 |          124 |          284 |        408 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1249 us
  parse       bench.c       :    330 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    205 us
  link        bench_rcc     :    183 us
  link        bench_rcc     :  78758 us

RCC -O1:
  preprocess  bench.c       :    830 us
  parse       bench.c       :    176 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    145 us
  link        bench_o1      :    187 us
  link        bench_o1      :  70569 us

RCC -O2:
  preprocess  bench.c       :   1744 us
  parse       bench.c       :    378 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     45 us
  codegen     bench.c       :    335 us
  link        bench_o2      :    222 us
  link        bench_o2      :  81589 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 325593 us
  parse       sqlite3.c     :  54548 us
  typecheck   sqlite3.c     :  13258 us
  codegen     sqlite3.c     : 103445 us
  link        sqlite3.so    :  16746 us

RCC -O1:
  preprocess  sqlite3.c     : 279272 us
  parse       sqlite3.c     :  58093 us
  typecheck   sqlite3.c     :  13164 us
  opt         sqlite3.c     :  20613 us
  codegen     sqlite3.c     : 106410 us
  link        sqlite3.so    :  17661 us

RCC -O2:
  preprocess  sqlite3.c     : 255048 us
  parse       sqlite3.c     :  52049 us
  typecheck   sqlite3.c     :  12454 us
  opt         sqlite3.c     : 154815 us
  codegen     sqlite3.c     : 100273 us
  link        sqlite3.so    :  16980 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       767 ms |
| RCC -O1   |       700 ms |
| RCC -O2   |       780 ms |
| TCC       |       115 ms |
| GCC -O0   |      1143 ms |
| GCC -O2   |     12364 ms |
| Clang -O0 |      1688 ms |
| Clang -O2 |     11466 ms |
