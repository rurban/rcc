# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          224 |          914 |       1138 |
| RCC -O1   |          140 |          839 |        979 |
| RCC -O2   |          133 |          852 |        985 |
| TCC       |          178 |          874 |       1052 |
| GCC -O0   |          245 |          669 |        914 |
| GCC -O2   |          194 |          362 |        556 |
| Clang -O0 |          174 |          614 |        788 |
| Clang -O2 |          205 |          386 |        591 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    851 us
  parse       bench.c       :    254 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    141 us
  link        bench_rcc     :    159 us
  link        bench_rcc     :  69244 us

RCC -O1:
  preprocess  bench.c       :    774 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    135 us
  link        bench_o1      :    325 us
  link        bench_o1      :  79851 us

RCC -O2:
  preprocess  bench.c       :    718 us
  parse       bench.c       :    240 us
  typecheck   bench.c       :     33 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    137 us
  link        bench_o2      :    188 us
  link        bench_o2      :  69051 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 409022 us
  parse       sqlite3.c     : 233111 us
  typecheck   sqlite3.c     :  27931 us
  codegen     sqlite3.c     : 220947 us
  link        sqlite3.so    :  21966 us

RCC -O1:
  preprocess  sqlite3.c     : 424143 us
  parse       sqlite3.c     :  82535 us
  typecheck   sqlite3.c     :  29770 us
  opt         sqlite3.c     :  38277 us
  codegen     sqlite3.c     : 271712 us
  link        sqlite3.so    :  25154 us

RCC -O2:
  preprocess  sqlite3.c     : 563231 us
  parse       sqlite3.c     : 139901 us
  typecheck   sqlite3.c     :  43584 us
  opt         sqlite3.c     : 296422 us
  codegen     sqlite3.c     : 301558 us
  link        sqlite3.so    :  26733 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1475 ms |
| RCC -O1   |      1307 ms |
| RCC -O2   |      1592 ms |
| TCC       |       304 ms |
| GCC -O0   |      1923 ms |
| GCC -O2   |     19835 ms |
| Clang -O0 |      2106 ms |
| Clang -O2 |     20245 ms |
