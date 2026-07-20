# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           42 |          590 |        632 |
| RCC -O1   |           40 |          598 |        638 |
| RCC -O2   |           42 |          617 |        659 |
| TCC       |            7 |          570 |        577 |
| SLIMCC    |           49 |          634 |        683 |
| XCC       |           11 |          363 |        374 |
| KEFIR     |          198 |          682 |        880 |
| KEFIR -O1 |          187 |          503 |        690 |
| CCC       |           37 |          575 |        612 |
| GCC -O0   |           74 |          577 |        651 |
| GCC -O2   |          184 |          213 |        397 |
| Clang -O0 |          117 |          660 |        777 |
| Clang -O2 |          154 |          242 |        396 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   6432 us
  parse       bench.c:    740 us
  typecheck   bench.c:     11 us
  codegen     bench.c:    690 us
  link        bench_rcc:  33367 us

RCC -O1:
  preprocess  bench.c:   4466 us
  parse       bench.c:    544 us
  typecheck   bench.c:      8 us
  opt         bench.c:     30 us
  codegen     bench.c:    434 us
  link        bench_o1:  31859 us

RCC -O2:
  preprocess  bench.c:   6496 us
  parse       bench.c:    667 us
  typecheck   bench.c:     11 us
  opt         bench.c:     37 us
  codegen     bench.c:    463 us
  link        bench_o2: 33877 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess sqlite3.c: 246923 us
  parse      sqlite3.c: 149304 us
  typecheck  sqlite3.c:  13753 us
  codegen    sqlite3.c: 135785 us

RCC -O1:
  preprocess sqlite3.c: 244627 us
  parse      sqlite3.c: 146589 us
  typecheck  sqlite3.c:  13739 us
  opt        sqlite3.c:  43002 us
  codegen    sqlite3.c: 120941 us

RCC -O2:
  preprocess sqlite3.c: 252336 us
  parse      sqlite3.c: 155225 us
  typecheck  sqlite3.c:  14817 us
  opt        sqlite3.c: 195203 us
  codegen    sqlite3.c: 120813 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       708 ms |
| RCC -O1   |       708 ms |
| RCC -O2   |       837 ms |
| TCC       |       112 ms |
| SLIMCC    |      1283 ms |
| KEFIR     |     35899 ms |
| KEFIR -O1 |     35150 ms |
| CCC       |     18021 ms |
| GCC -O0   |     11847 ms |
| GCC -O2   |     94069 ms |
| Clang -O0 |      4221 ms |
| Clang -O2 |     43760 ms |
