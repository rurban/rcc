# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           65 |          618 |        683 |
| RCC -O1   |           62 |          622 |        684 |
| TCC       |           37 |          513 |        550 |
| GCC -O0   |           64 |          434 |        498 |
| GCC -O2   |           81 |          262 |        343 |
| Clang -O0 |           50 |          433 |        483 |
| Clang -O2 |           74 |          262 |        336 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    613 us
  lex         bench.c:     72 us
  parse       bench.c:     74 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   1485 us
  peephole    bench.c:    236 us
  link        bench_rcc:  60437 us

RCC -O1:
  preprocess  bench.c:    578 us
  lex         bench.c:     73 us
  parse       bench.c:     73 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:   1323 us
  peephole    bench.c:    222 us
  link        bench_rcc_o1:  56804 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 309878 us
  lex         sqlite3.c:  74294 us
  parse       sqlite3.c:  68995 us
  typecheck   sqlite3.c:  19750 us
  codegen     sqlite3.c: 574474 us
  peephole    sqlite3.c: 160209 us
  link        null: 4928495 us

RCC -O1:
  preprocess  sqlite3.c: 227600 us
  lex         sqlite3.c:  58130 us
  parse       sqlite3.c:  50356 us
  typecheck   sqlite3.c:  14349 us
  opt(CTFE)   sqlite3.c:  18982 us
  codegen     sqlite3.c: 447107 us
  peephole    sqlite3.c: 150348 us
  link        null: 4839190 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      5903 ms |
| RCC -O1   |      5941 ms |
| TCC       |        77 ms |
| GCC -O0   |       822 ms |
| GCC -O2   |      8335 ms |
| Clang -O0 |       923 ms |
| Clang -O2 |      8297 ms |
