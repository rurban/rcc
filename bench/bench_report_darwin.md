# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          183 |          831 |       1014 |
| RCC -O1   |          110 |          834 |        944 |
| RCC -O2   |          121 |          859 |        980 |
| TCC       |          120 |          702 |        822 |
| GCC -O0   |          186 |          632 |        818 |
| GCC -O2   |          196 |          394 |        590 |
| Clang -O0 |          119 |          591 |        710 |
| Clang -O2 |          169 |          355 |        524 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    861 us
  parse       bench.c       :    448 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    124 us
  link        bench_rcc     :    826 us
  link        bench_rcc     :  79597 us

RCC -O1:
  preprocess  bench.c       :    683 us
  parse       bench.c       :    189 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    114 us
  link        bench_o1      :     77 us
  link        bench_o1      :  92695 us

RCC -O2:
  preprocess  bench.c       :   2462 us
  parse       bench.c       :    195 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    220 us
  link        bench_o2      :     80 us
  link        bench_o2      : 103992 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 570566 us
  parse       sqlite3.c     : 209864 us
  typecheck   sqlite3.c     :  43801 us
  codegen     sqlite3.c     : 232796 us
  link        sqlite3.so    :  26303 us

RCC -O1:
  preprocess  sqlite3.c     : 464892 us
  parse       sqlite3.c     :  88431 us
  typecheck   sqlite3.c     :  35078 us
  opt         sqlite3.c     :  40959 us
  codegen     sqlite3.c     : 194411 us
  link        sqlite3.so    :  22855 us

RCC -O2:
  preprocess  sqlite3.c     : 513165 us
  parse       sqlite3.c     :  72470 us
  typecheck   sqlite3.c     :  26324 us
  opt         sqlite3.c     : 339464 us
  codegen     sqlite3.c     : 215891 us
  link        sqlite3.so    :  19214 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1505 ms |
| RCC -O1   |      1088 ms |
| RCC -O2   |      1314 ms |
| TCC       |       204 ms |
| GCC -O0   |      2207 ms |
| GCC -O2   |     17850 ms |
| Clang -O0 |      1907 ms |
| Clang -O2 |     17009 ms |
