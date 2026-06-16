# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          104 |          682 |        786 |
| RCC -O1   |          120 |          664 |        784 |
| TCC       |           99 |          561 |        660 |
| GCC -O0   |           95 |          481 |        576 |
| GCC -O2   |          148 |          294 |        442 |
| Clang -O0 |           97 |          482 |        579 |
| Clang -O2 |          147 |          283 |        430 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1844 us
  lex         bench.c:    109 us
  parse       bench.c:    179 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   2649 us
  peephole    bench.c:    242 us
  link        bench_rcc:  72802 us

RCC -O1:
  preprocess  bench.c:   1516 us
  lex         bench.c:    105 us
  parse       bench.c:    102 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:   2022 us
  peephole    bench.c:    260 us
  link        bench_rcc_o1:  68799 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 515802 us
  lex         sqlite3.c: 173009 us
  parse       sqlite3.c: 137829 us
  typecheck   sqlite3.c:  22003 us
  codegen     sqlite3.c: 696064 us
  peephole    sqlite3.c: 175548 us
  link        null: 5611578 us

RCC -O1:
  preprocess  sqlite3.c: 580722 us
  lex         sqlite3.c: 156527 us
  parse       sqlite3.c:  77096 us
  typecheck   sqlite3.c:  14499 us
  opt(CTFE)   sqlite3.c:  25309 us
  codegen     sqlite3.c: 909416 us
  peephole    sqlite3.c: 199059 us
  link        null: 5691710 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      7921 ms |
| RCC -O1   |      8299 ms |
| TCC       |       184 ms |
| GCC -O0   |      1466 ms |
| GCC -O2   |     15837 ms |
| Clang -O0 |      1389 ms |
| Clang -O2 |     16464 ms |
