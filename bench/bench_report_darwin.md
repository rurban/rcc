# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          148 |          901 |       1049 |
| RCC -O1   |          104 |          775 |        879 |
| TCC       |           73 |          736 |        809 |
| GCC -O0   |          145 |          630 |        775 |
| GCC -O2   |          190 |          341 |        531 |
| Clang -O0 |           95 |          535 |        630 |
| Clang -O2 |          117 |          330 |        447 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1788 us
  lex         bench.c:     87 us
  parse       bench.c:     96 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    131 us
  peephole    bench.c:     57 us
  link        bench_rcc:  57465 us

RCC -O1:
  preprocess  bench.c:   2422 us
  lex         bench.c:     85 us
  parse       bench.c:    107 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    125 us
  peephole    bench.c:     62 us
  link        bench_rcc_o1:  61472 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1677209 us
  lex         sqlite3.c:  95282 us
  parse       sqlite3.c:  96023 us
  typecheck   sqlite3.c:  20504 us
  codegen     sqlite3.c: 487581 us
  peephole    sqlite3.c: 2722689 us

RCC -O1:
  preprocess  sqlite3.c: 1242803 us
  lex         sqlite3.c:  97278 us
  parse       sqlite3.c:  97058 us
  typecheck   sqlite3.c:  20253 us
  opt(CTFE)   sqlite3.c:  22795 us
  codegen     sqlite3.c: 467318 us
  peephole    sqlite3.c: 2522022 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      5419 ms |
| RCC -O1   |      5556 ms |
| TCC       |       116 ms |
| GCC -O0   |      1357 ms |
| GCC -O2   |     14284 ms |
| Clang -O0 |      1501 ms |
| Clang -O2 |     13633 ms |
