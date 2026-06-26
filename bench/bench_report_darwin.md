# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           72 |          570 |        642 |
| RCC -O1   |           48 |          624 |        672 |
| TCC       |           37 |          514 |        551 |
| GCC -O0   |           71 |          434 |        505 |
| GCC -O2   |           96 |          262 |        358 |
| Clang -O0 |           58 |          433 |        491 |
| Clang -O2 |           79 |          262 |        341 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1023 us
  lex         bench.c:     74 us
  parse       bench.c:     87 us
  typecheck   bench.c:      5 us
  codegen     bench.c:     86 us
  peephole    bench.c:     48 us
  link        bench_rcc:  49070 us

RCC -O1:
  preprocess  bench.c:   1145 us
  lex         bench.c:     74 us
  parse       bench.c:     75 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:     79 us
  peephole    bench.c:     48 us
  link        bench_rcc_o1:  43868 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1079115 us
  lex         sqlite3.c:  84307 us
  parse       sqlite3.c:  78268 us
  typecheck   sqlite3.c:  19099 us
  codegen     sqlite3.c: 387002 us
  peephole    sqlite3.c: 2138753 us

RCC -O1:
  preprocess  sqlite3.c: 641131 us
  lex         sqlite3.c:  80809 us
  parse       sqlite3.c:  83193 us
  typecheck   sqlite3.c:  16243 us
  opt(CTFE)   sqlite3.c:  17332 us
  codegen     sqlite3.c: 378279 us
  peephole    sqlite3.c: 2087359 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      3572 ms |
| RCC -O1   |      3257 ms |
| TCC       |        74 ms |
| GCC -O0   |       871 ms |
| GCC -O2   |      8429 ms |
| Clang -O0 |       837 ms |
| Clang -O2 |      8414 ms |
