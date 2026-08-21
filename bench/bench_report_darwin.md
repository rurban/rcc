# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           99 |          713 |        812 |
| RCC -O1   |           72 |          756 |        828 |
| RCC -O2   |          117 |          809 |        926 |
| TCC       |           62 |          711 |        773 |
| GCC -O0   |          137 |          702 |        839 |
| GCC -O2   |          223 |          393 |        616 |
| Clang -O0 |          123 |          525 |        648 |
| Clang -O2 |          125 |          330 |        455 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    955 us
  parse       bench.c       :    196 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    145 us
  link        bench_rcc     :    432 us
  link        bench_rcc     :  75848 us

RCC -O1:
  preprocess  bench.c       :    675 us
  parse       bench.c       :    156 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    138 us
  link        bench_o1      :     87 us
  link        bench_o1      :  57205 us

RCC -O2:
  preprocess  bench.c       :    661 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    135 us
  link        bench_o2      :    227 us
  link        bench_o2      :  52168 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 324462 us
  parse       sqlite3.c     : 164994 us
  typecheck   sqlite3.c     :  28128 us
  codegen     sqlite3.c     : 173934 us
  link        sqlite3.so    :  21557 us

RCC -O1:
  preprocess  sqlite3.c     : 409891 us
  parse       sqlite3.c     :  63124 us
  typecheck   sqlite3.c     :  18594 us
  opt         sqlite3.c     : 172590 us
  codegen     sqlite3.c     : 174605 us
  link        sqlite3.so    :  16438 us

RCC -O2:
  preprocess  sqlite3.c     : 290654 us
  parse       sqlite3.c     :  62096 us
  typecheck   sqlite3.c     :  17504 us
  opt         sqlite3.c     : 348641 us
  codegen     sqlite3.c     : 140778 us
  link        sqlite3.so    :  19231 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1023 ms |
| RCC -O1   |      1117 ms |
| RCC -O2   |      1189 ms |
| TCC       |       151 ms |
| GCC -O0   |      1215 ms |
| GCC -O2   |     12299 ms |
| Clang -O0 |      1700 ms |
| Clang -O2 |     14231 ms |
