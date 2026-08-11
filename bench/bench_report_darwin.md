# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          642 |        695 |
| RCC -O1   |           51 |          624 |        675 |
| RCC -O2   |           76 |          583 |        659 |
| TCC       |           41 |          515 |        556 |
| GCC -O0   |           85 |          436 |        521 |
| GCC -O2   |           94 |          271 |        365 |
| Clang -O0 |           89 |          481 |        570 |
| Clang -O2 |          134 |          263 |        397 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1001 us
  parse       bench.c       :    183 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    366 us
  link        bench_rcc     :    406 us
  link        bench_rcc     :  83094 us

RCC -O1:
  preprocess  bench.c       :    767 us
  parse       bench.c       :    179 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    143 us
  link        bench_o1      :    173 us
  link        bench_o1      :  67055 us

RCC -O2:
  preprocess  bench.c       :    830 us
  parse       bench.c       :    188 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    156 us
  link        bench_o2      :    187 us
  link        bench_o2      :  68380 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 337131 us
  parse       sqlite3.c     :  56129 us
  typecheck   sqlite3.c     :  11844 us
  codegen     sqlite3.c     : 127995 us
  link        sqlite3.so    :  16200 us

RCC -O1:
  preprocess  sqlite3.c     : 333434 us
  parse       sqlite3.c     :  56308 us
  typecheck   sqlite3.c     :  14995 us
  opt         sqlite3.c     :  23106 us
  codegen     sqlite3.c     :  93696 us
  link        sqlite3.so    :  14940 us

RCC -O2:
  preprocess  sqlite3.c     : 225106 us
  parse       sqlite3.c     :  49349 us
  typecheck   sqlite3.c     :  11707 us
  opt         sqlite3.c     : 119779 us
  codegen     sqlite3.c     :  98723 us
  link        sqlite3.so    :  16380 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       648 ms |
| RCC -O1   |       675 ms |
| RCC -O2   |       647 ms |
| TCC       |       105 ms |
| GCC -O0   |       946 ms |
| GCC -O2   |     10354 ms |
| Clang -O0 |      1108 ms |
| Clang -O2 |      9145 ms |
