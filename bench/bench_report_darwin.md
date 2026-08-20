# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           82 |          722 |        804 |
| RCC -O1   |          129 |          819 |        948 |
| RCC -O2   |          136 |          850 |        986 |
| TCC       |           78 |          620 |        698 |
| GCC -O0   |          122 |          564 |        686 |
| GCC -O2   |          327 |          354 |        681 |
| Clang -O0 |          130 |          524 |        654 |
| Clang -O2 |          144 |          322 |        466 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    916 us
  parse       bench.c       :    196 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    173 us
  link        bench_rcc     :    140 us
  link        bench_rcc     :  61674 us

RCC -O1:
  preprocess  bench.c       :    664 us
  parse       bench.c       :    144 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    133 us
  link        bench_o1      :    205 us
  link        bench_o1      :  62665 us

RCC -O2:
  preprocess  bench.c       :    706 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    126 us
  link        bench_o2      :    114 us
  link        bench_o2      :  82031 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 308609 us
  parse       sqlite3.c     : 166546 us
  typecheck   sqlite3.c     :  30211 us
  codegen     sqlite3.c     : 149838 us
  link        sqlite3.so    :  20572 us

RCC -O1:
  preprocess  sqlite3.c     : 285116 us
  parse       sqlite3.c     :  63814 us
  typecheck   sqlite3.c     :  15946 us
  opt         sqlite3.c     : 180711 us
  codegen     sqlite3.c     : 200916 us
  link        sqlite3.so    :  15870 us

RCC -O2:
  preprocess  sqlite3.c     : 309374 us
  parse       sqlite3.c     :  58414 us
  typecheck   sqlite3.c     :  20664 us
  opt         sqlite3.c     : 194919 us
  codegen     sqlite3.c     : 151694 us
  link        sqlite3.so    :  17967 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1249 ms |
| RCC -O1   |      1056 ms |
| RCC -O2   |      1212 ms |
| TCC       |       189 ms |
| GCC -O0   |      1479 ms |
| GCC -O2   |     12665 ms |
| Clang -O0 |      1392 ms |
| Clang -O2 |     16868 ms |
