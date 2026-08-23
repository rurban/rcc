# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          107 |          722 |        829 |
| RCC -O1   |           74 |          710 |        784 |
| RCC -O2   |           73 |          659 |        732 |
| TCC       |           52 |          640 |        692 |
| GCC -O0   |          100 |          537 |        637 |
| GCC -O2   |          143 |          322 |        465 |
| Clang -O0 |           86 |          659 |        745 |
| Clang -O2 |          138 |          384 |        522 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1428 us
  parse       bench.c       :    246 us
  typecheck   bench.c       :     22 us
  codegen     bench.c       :    214 us
  link        bench_rcc     :    417 us
  link        bench_rcc     :  66650 us

RCC -O1:
  preprocess  bench.c       :    722 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    175 us
  link        bench_o1      :    767 us
  link        bench_o1      :  68754 us

RCC -O2:
  preprocess  bench.c       :    783 us
  parse       bench.c       :    258 us
  typecheck   bench.c       :      8 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    159 us
  link        bench_o2      :    180 us
  link        bench_o2      :  70987 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 401522 us
  parse       sqlite3.c     : 162999 us
  typecheck   sqlite3.c     :  27161 us
  codegen     sqlite3.c     : 155675 us
  link        sqlite3.so    :  24559 us

RCC -O1:
  preprocess  sqlite3.c     : 355798 us
  parse       sqlite3.c     :  78414 us
  typecheck   sqlite3.c     :  17872 us
  opt         sqlite3.c     : 214533 us
  codegen     sqlite3.c     : 204814 us
  link        sqlite3.so    :  19906 us

RCC -O2:
  preprocess  sqlite3.c     : 352114 us
  parse       sqlite3.c     :  70240 us
  typecheck   sqlite3.c     :  17580 us
  opt         sqlite3.c     : 210227 us
  codegen     sqlite3.c     : 170472 us
  link        sqlite3.so    :  19869 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1281 ms |
| RCC -O1   |      1175 ms |
| RCC -O2   |       990 ms |
| TCC       |       175 ms |
| GCC -O0   |      1808 ms |
| GCC -O2   |     13922 ms |
| Clang -O0 |      1324 ms |
| Clang -O2 |     12814 ms |
