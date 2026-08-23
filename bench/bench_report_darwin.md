# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          109 |          752 |        861 |
| RCC -O1   |          104 |          782 |        886 |
| RCC -O2   |          141 |          780 |        921 |
| TCC       |           68 |          583 |        651 |
| GCC -O0   |           83 |          499 |        582 |
| GCC -O2   |          154 |          340 |        494 |
| Clang -O0 |           82 |          594 |        676 |
| Clang -O2 |          159 |          383 |        542 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2346 us
  parse       bench.c       :    376 us
  typecheck   bench.c       :     10 us
  codegen     bench.c       :    363 us
  link        bench_rcc     :    230 us
  link        bench_rcc     :  85161 us

RCC -O1:
  preprocess  bench.c       :    806 us
  parse       bench.c       :    219 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    166 us
  link        bench_o1      :    220 us
  link        bench_o1      :  83675 us

RCC -O2:
  preprocess  bench.c       :    789 us
  parse       bench.c       :    215 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    199 us
  link        bench_o2      :    203 us
  link        bench_o2      :  74932 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 394261 us
  parse       sqlite3.c     :  82388 us
  typecheck   sqlite3.c     :  22985 us
  codegen     sqlite3.c     : 146114 us
  link        sqlite3.so    :  19856 us

RCC -O1:
  preprocess  sqlite3.c     : 344524 us
  parse       sqlite3.c     :  75744 us
  typecheck   sqlite3.c     :  13775 us
  opt         sqlite3.c     : 229825 us
  codegen     sqlite3.c     : 110384 us
  link        sqlite3.so    :  18174 us

RCC -O2:
  preprocess  sqlite3.c     : 291572 us
  parse       sqlite3.c     :  65669 us
  typecheck   sqlite3.c     :  15725 us
  opt         sqlite3.c     : 247893 us
  codegen     sqlite3.c     : 179476 us
  link        sqlite3.so    :  18966 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1202 ms |
| RCC -O1   |      1288 ms |
| RCC -O2   |      1547 ms |
| TCC       |       184 ms |
| GCC -O0   |      2114 ms |
| GCC -O2   |     22290 ms |
| Clang -O0 |      2048 ms |
| Clang -O2 |     16703 ms |
