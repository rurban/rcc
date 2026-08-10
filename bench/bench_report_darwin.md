# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          137 |          732 |        869 |
| RCC -O1   |           69 |          718 |        787 |
| RCC -O2   |           79 |          710 |        789 |
| TCC       |           62 |          632 |        694 |
| GCC -O0   |          106 |          488 |        594 |
| GCC -O2   |          119 |          291 |        410 |
| Clang -O0 |           74 |          486 |        560 |
| Clang -O2 |           95 |          293 |        388 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1054 us
  parse       bench.c       :    199 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    219 us
  link        bench_rcc     :    888 us
  link        bench_rcc     :  74990 us

RCC -O1:
  preprocess  bench.c       :    992 us
  parse       bench.c       :    183 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    151 us
  link        bench_o1      :    353 us
  link        bench_o1      :  92849 us

RCC -O2:
  preprocess  bench.c       :    927 us
  parse       bench.c       :    331 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    153 us
  link        bench_o2      :    184 us
  link        bench_o2      :  83239 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 335212 us
  parse       sqlite3.c     : 131200 us
  typecheck   sqlite3.c     :  23604 us
  codegen     sqlite3.c     : 120418 us
  link        sqlite3.so    :  19366 us

RCC -O1:
  preprocess  sqlite3.c     : 362584 us
  parse       sqlite3.c     :  61590 us
  typecheck   sqlite3.c     :  13742 us
  opt         sqlite3.c     :  21060 us
  codegen     sqlite3.c     : 117638 us
  link        sqlite3.so    :  31831 us

RCC -O2:
  preprocess  sqlite3.c     : 324800 us
  parse       sqlite3.c     :  52299 us
  typecheck   sqlite3.c     :  12669 us
  opt         sqlite3.c     : 145227 us
  codegen     sqlite3.c     : 101967 us
  link        sqlite3.so    :  17469 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1144 ms |
| RCC -O1   |      1103 ms |
| RCC -O2   |       996 ms |
| TCC       |       122 ms |
| GCC -O0   |      1225 ms |
| GCC -O2   |     13304 ms |
| Clang -O0 |      1445 ms |
| Clang -O2 |     12201 ms |
