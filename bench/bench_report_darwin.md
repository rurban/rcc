# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          138 |          647 |        785 |
| RCC -O1   |           90 |          655 |        745 |
| TCC       |          103 |          601 |        704 |
| GCC -O0   |          143 |          509 |        652 |
| GCC -O2   |          146 |          298 |        444 |
| Clang -O0 |           97 |          477 |        574 |
| Clang -O2 |          110 |          315 |        425 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1294 us
  lex         bench.c:     79 us
  parse       bench.c:     90 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   1312 us
  peephole    bench.c:    292 us (est, 779 calls, pat3=0)
  link        bench_rcc:  92120 us

RCC -O1:
  preprocess  bench.c:   2968 us
  lex         bench.c:     90 us
  parse       bench.c:     89 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:   5629 us
  peephole    bench.c:    291 us (est, 776 calls, pat3=0)
  link        bench_rcc_o1:  89800 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1630488 us
  lex         sqlite3.c:  97511 us
  parse       sqlite3.c:  79470 us
  typecheck   sqlite3.c:  26361 us
  codegen     sqlite3.c: 704589 us
  peephole    sqlite3.c: 203976 us (est, 543938 calls, pat3=66)
  link        null: 6756264 us

RCC -O1:
  preprocess  sqlite3.c: 1841730 us
  lex         sqlite3.c:  96770 us
  parse       sqlite3.c:  95994 us
  typecheck   sqlite3.c:  26278 us
  opt(CTFE)   sqlite3.c:  63484 us
  codegen     sqlite3.c: 1326507 us
  peephole    sqlite3.c: 202746 us (est, 540657 calls, pat3=66)
  link        null: 6671747 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      9928 ms |
| RCC -O1   |     10361 ms |
| TCC       |       389 ms |
| GCC -O0   |      1285 ms |
| GCC -O2   |     12074 ms |
| Clang -O0 |       964 ms |
| Clang -O2 |     11199 ms |
