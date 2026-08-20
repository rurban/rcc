# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          142 |          833 |        975 |
| RCC -O1   |           96 |          776 |        872 |
| RCC -O2   |           90 |          819 |        909 |
| TCC       |          125 |          638 |        763 |
| GCC -O0   |          101 |          518 |        619 |
| GCC -O2   |          119 |          306 |        425 |
| Clang -O0 |           73 |          495 |        568 |
| Clang -O2 |          116 |          304 |        420 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    795 us
  parse       bench.c       :    196 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    181 us
  link        bench_rcc     :    247 us
  link        bench_rcc     :  87745 us

RCC -O1:
  preprocess  bench.c       :    779 us
  parse       bench.c       :    319 us
  typecheck   bench.c       :      7 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    147 us
  link        bench_o1      :    181 us
  link        bench_o1      :  85947 us

RCC -O2:
  preprocess  bench.c       :    650 us
  parse       bench.c       :    164 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    160 us
  link        bench_o2      :    216 us
  link        bench_o2      :  72786 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 337027 us
  parse       sqlite3.c     : 165227 us
  typecheck   sqlite3.c     :  54266 us
  codegen     sqlite3.c     : 147875 us
  link        sqlite3.so    :  24759 us

RCC -O1:
  preprocess  sqlite3.c     : 337648 us
  parse       sqlite3.c     :  90163 us
  typecheck   sqlite3.c     :  30314 us
  opt         sqlite3.c     : 253311 us
  codegen     sqlite3.c     : 170843 us
  link        sqlite3.so    :  17150 us

RCC -O2:
  preprocess  sqlite3.c     : 276382 us
  parse       sqlite3.c     :  80333 us
  typecheck   sqlite3.c     :  15667 us
  opt         sqlite3.c     : 219001 us
  codegen     sqlite3.c     : 268999 us
  link        sqlite3.so    :  25354 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1087 ms |
| RCC -O1   |       832 ms |
| RCC -O2   |       876 ms |
| TCC       |        94 ms |
| GCC -O0   |      1169 ms |
| GCC -O2   |     12029 ms |
| Clang -O0 |      1511 ms |
| Clang -O2 |     16251 ms |
