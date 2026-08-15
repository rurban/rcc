# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           50 |          626 |        676 |
| RCC -O1   |           52 |          645 |        697 |
| RCC -O2   |           78 |          660 |        738 |
| TCC       |           49 |          579 |        628 |
| GCC -O0   |          111 |          519 |        630 |
| GCC -O2   |          125 |          322 |        447 |
| Clang -O0 |          118 |          510 |        628 |
| Clang -O2 |          143 |          308 |        451 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    708 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    126 us
  link        bench_rcc     :     90 us
  link        bench_rcc     :  50175 us

RCC -O1:
  preprocess  bench.c       :    668 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    128 us
  link        bench_o1      :    179 us
  link        bench_o1      :  48061 us

RCC -O2:
  preprocess  bench.c       :    618 us
  parse       bench.c       :    124 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    122 us
  link        bench_o2      :    367 us
  link        bench_o2      :  53839 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 260890 us
  parse       sqlite3.c     :  53014 us
  typecheck   sqlite3.c     :  12616 us
  codegen     sqlite3.c     : 104914 us
  link        sqlite3.so    :  14092 us

RCC -O1:
  preprocess  sqlite3.c     : 207726 us
  parse       sqlite3.c     :  51169 us
  typecheck   sqlite3.c     :  12463 us
  opt         sqlite3.c     : 140216 us
  codegen     sqlite3.c     :  91101 us
  link        sqlite3.so    :  14197 us

RCC -O2:
  preprocess  sqlite3.c     : 192292 us
  parse       sqlite3.c     :  46922 us
  typecheck   sqlite3.c     :  12119 us
  opt         sqlite3.c     : 136019 us
  codegen     sqlite3.c     :  87908 us
  link        sqlite3.so    :  14533 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       986 ms |
| RCC -O1   |      1092 ms |
| RCC -O2   |      1038 ms |
| TCC       |       164 ms |
| GCC -O0   |      1816 ms |
| GCC -O2   |     12967 ms |
| Clang -O0 |      1269 ms |
| Clang -O2 |     12623 ms |
