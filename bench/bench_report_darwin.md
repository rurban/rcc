# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           79 |          691 |        770 |
| RCC -O1   |           83 |          684 |        767 |
| RCC -O2   |           70 |          686 |        756 |
| TCC       |           58 |          602 |        660 |
| GCC -O0   |           88 |          501 |        589 |
| GCC -O2   |          158 |          331 |        489 |
| Clang -O0 |          100 |          522 |        622 |
| Clang -O2 |          119 |          304 |        423 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    844 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    158 us
  link        bench_rcc     :    263 us
  link        bench_rcc     :  66183 us

RCC -O1:
  preprocess  bench.c       :    745 us
  parse       bench.c       :    150 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    145 us
  link        bench_o1      :    656 us
  link        bench_o1      :  64863 us

RCC -O2:
  preprocess  bench.c       :    711 us
  parse       bench.c       :    188 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    124 us
  link        bench_o2      :    227 us
  link        bench_o2      :  67501 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 331872 us
  parse       sqlite3.c     :  69147 us
  typecheck   sqlite3.c     :  14561 us
  codegen     sqlite3.c     : 115507 us
  link        sqlite3.so    :  15879 us

RCC -O1:
  preprocess  sqlite3.c     : 260031 us
  parse       sqlite3.c     :  64573 us
  typecheck   sqlite3.c     :  14886 us
  opt         sqlite3.c     : 146420 us
  codegen     sqlite3.c     : 112936 us
  link        sqlite3.so    :  17352 us

RCC -O2:
  preprocess  sqlite3.c     : 281516 us
  parse       sqlite3.c     :  71197 us
  typecheck   sqlite3.c     :  16885 us
  opt         sqlite3.c     : 199963 us
  codegen     sqlite3.c     : 144175 us
  link        sqlite3.so    :  18378 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       675 ms |
| RCC -O1   |       823 ms |
| RCC -O2   |       870 ms |
| TCC       |       118 ms |
| GCC -O0   |      1205 ms |
| GCC -O2   |     13661 ms |
| Clang -O0 |      1389 ms |
| Clang -O2 |     11906 ms |
