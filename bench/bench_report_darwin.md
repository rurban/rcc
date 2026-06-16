# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          138 |          805 |        943 |
| RCC -O1   |          152 |          797 |        949 |
| TCC       |           81 |          701 |        782 |
| GCC -O0   |          158 |          494 |        652 |
| GCC -O2   |          147 |          293 |        440 |
| Clang -O0 |          121 |          491 |        612 |
| Clang -O2 |          125 |          293 |        418 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    962 us
  lex         bench.c:    109 us
  parse       bench.c:    142 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   2357 us
  peephole    bench.c:    400 us
  link        bench_rcc:  95707 us

RCC -O1:
  preprocess  bench.c:   1054 us
  lex         bench.c:    146 us
  parse       bench.c:     99 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     23 us
  codegen     bench.c:   5559 us
  peephole    bench.c:    553 us
  link        bench_rcc_o1: 100056 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1002558 us
  lex         sqlite3.c: 176752 us
  parse       sqlite3.c: 188290 us
  typecheck   sqlite3.c:  26823 us
  codegen     sqlite3.c: 1043073 us
  peephole    sqlite3.c: 216759 us
  link        null: 6893564 us

RCC -O1:
  preprocess  sqlite3.c: 913661 us
  lex         sqlite3.c: 166170 us
  parse       sqlite3.c: 206096 us
  typecheck   sqlite3.c:  24060 us
  opt(CTFE)   sqlite3.c:  24735 us
  codegen     sqlite3.c: 1414906 us
  peephole    sqlite3.c: 290507 us
  link        null: 7737225 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      8044 ms |
| RCC -O1   |      7539 ms |
| TCC       |       106 ms |
| GCC -O0   |      1069 ms |
| GCC -O2   |     13744 ms |
| Clang -O0 |      1728 ms |
| Clang -O2 |     13692 ms |
