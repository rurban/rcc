# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           55 |          643 |        698 |
| RCC -O1   |           54 |          643 |        697 |
| RCC -O2   |           56 |          653 |        709 |
| TCC       |           51 |          555 |        606 |
| GCC -O0   |           73 |          467 |        540 |
| GCC -O2   |          109 |          280 |        389 |
| Clang -O0 |           58 |          465 |        523 |
| Clang -O2 |           84 |          281 |        365 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    820 us
  parse       bench.c       :    184 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    175 us
  link        bench_rcc     :    118 us
  link        bench_rcc     :  51514 us

RCC -O1:
  preprocess  bench.c       :    743 us
  parse       bench.c       :    191 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    224 us
  link        bench_o1      :    165 us
  link        bench_o1      :  56000 us

RCC -O2:
  preprocess  bench.c       :    702 us
  parse       bench.c       :    184 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    151 us
  link        bench_o2      :    146 us
  link        bench_o2      :  53725 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 278029 us
  parse       sqlite3.c     :  59138 us
  typecheck   sqlite3.c     :  13675 us
  codegen     sqlite3.c     :  98290 us
  link        sqlite3.so    :  15839 us

RCC -O1:
  preprocess  sqlite3.c     : 213972 us
  parse       sqlite3.c     :  52460 us
  typecheck   sqlite3.c     :  13071 us
  opt         sqlite3.c     : 179109 us
  codegen     sqlite3.c     : 123387 us
  link        sqlite3.so    :  15309 us

RCC -O2:
  preprocess  sqlite3.c     : 236173 us
  parse       sqlite3.c     :  56932 us
  typecheck   sqlite3.c     :  14146 us
  opt         sqlite3.c     : 161430 us
  codegen     sqlite3.c     : 100123 us
  link        sqlite3.so    :  16068 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       567 ms |
| RCC -O1   |       802 ms |
| RCC -O2   |       709 ms |
| TCC       |       100 ms |
| GCC -O0   |      1013 ms |
| GCC -O2   |      9716 ms |
| Clang -O0 |      1061 ms |
| Clang -O2 |      9998 ms |
