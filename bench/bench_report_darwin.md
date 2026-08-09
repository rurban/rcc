# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           75 |          612 |        687 |
| RCC -O1   |           54 |          617 |        671 |
| RCC -O2   |           61 |          631 |        692 |
| TCC       |           57 |          579 |        636 |
| GCC -O0   |          200 |          488 |        688 |
| GCC -O2   |          104 |          292 |        396 |
| Clang -O0 |           55 |          485 |        540 |
| Clang -O2 |          104 |          274 |        378 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1014 us
  parse       bench.c       :    207 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    447 us
  link        bench_rcc     :    377 us
  link        bench_rcc     :  61157 us

RCC -O1:
  preprocess  bench.c       :    718 us
  parse       bench.c       :    195 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    158 us
  link        bench_o1      :    131 us
  link        bench_o1      :  54981 us

RCC -O2:
  preprocess  bench.c       :    768 us
  parse       bench.c       :    184 us
  typecheck   bench.c       :      8 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    142 us
  link        bench_o2      :    192 us
  link        bench_o2      :  56875 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 266197 us
  parse       sqlite3.c     :  58739 us
  typecheck   sqlite3.c     :  14227 us
  codegen     sqlite3.c     : 144632 us
  link        sqlite3.so    :  23950 us

RCC -O1:
  preprocess  sqlite3.c     : 238255 us
  parse       sqlite3.c     :  53925 us
  typecheck   sqlite3.c     :  13038 us
  opt         sqlite3.c     :  18806 us
  codegen     sqlite3.c     : 110937 us
  link        sqlite3.so    :  14335 us

RCC -O2:
  preprocess  sqlite3.c     : 244222 us
  parse       sqlite3.c     :  52558 us
  typecheck   sqlite3.c     :  13709 us
  opt         sqlite3.c     : 147175 us
  codegen     sqlite3.c     :  99520 us
  link        sqlite3.so    :  13816 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       841 ms |
| RCC -O1   |       718 ms |
| RCC -O2   |       737 ms |
| TCC       |       102 ms |
| GCC -O0   |      1071 ms |
| GCC -O2   |      9884 ms |
| Clang -O0 |      1097 ms |
| Clang -O2 |     10533 ms |
