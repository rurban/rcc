# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          133 |          776 |        909 |
| RCC -O1   |           89 |          774 |        863 |
| RCC -O2   |           81 |          761 |        842 |
| TCC       |           77 |          611 |        688 |
| GCC -O0   |          107 |          549 |        656 |
| GCC -O2   |          156 |          358 |        514 |
| Clang -O0 |           78 |          582 |        660 |
| Clang -O2 |          210 |          356 |        566 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    772 us
  parse       bench.c       :    130 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    134 us
  link        bench_rcc     :    162 us
  link        bench_rcc     : 115388 us

RCC -O1:
  preprocess  bench.c       :    987 us
  parse       bench.c       :    304 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    191 us
  link        bench_o1      :   1119 us
  link        bench_o1      :  69624 us

RCC -O2:
  preprocess  bench.c       :    766 us
  parse       bench.c       :    175 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    155 us
  link        bench_o2      :    237 us
  link        bench_o2      :  60978 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 329227 us
  parse       sqlite3.c     : 182966 us
  typecheck   sqlite3.c     :  30260 us
  codegen     sqlite3.c     : 189508 us
  link        sqlite3.so    :  22912 us

RCC -O1:
  preprocess  sqlite3.c     : 371844 us
  parse       sqlite3.c     :  68826 us
  typecheck   sqlite3.c     :  15761 us
  opt         sqlite3.c     : 183244 us
  codegen     sqlite3.c     : 121651 us
  link        sqlite3.so    :  15319 us

RCC -O2:
  preprocess  sqlite3.c     : 234417 us
  parse       sqlite3.c     :  53893 us
  typecheck   sqlite3.c     :  15220 us
  opt         sqlite3.c     : 285476 us
  codegen     sqlite3.c     : 159024 us
  link        sqlite3.so    :  17126 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1822 ms |
| RCC -O1   |      1204 ms |
| RCC -O2   |      1382 ms |
| TCC       |       205 ms |
| GCC -O0   |      2116 ms |
| GCC -O2   |     12842 ms |
| Clang -O0 |      1273 ms |
| Clang -O2 |     10258 ms |
