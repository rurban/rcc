# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           70 |          608 |        678 |
| RCC -O1   |           65 |          594 |        659 |
| TCC       |           33 |          531 |        564 |
| GCC -O0   |           65 |          450 |        515 |
| GCC -O2   |           86 |          271 |        357 |
| Clang -O0 |           52 |          451 |        503 |
| Clang -O2 |           76 |          273 |        349 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    638 us
  lex         bench.c:     99 us
  parse       bench.c:     60 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1206 us
  peephole    bench.c:    217 us
  link        bench_rcc:  58936 us

RCC -O1:
  preprocess  bench.c:    638 us
  lex         bench.c:     99 us
  parse       bench.c:     60 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1411 us
  peephole    bench.c:    230 us
  link        bench_rcc_o1:  66762 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 359998 us
  lex         sqlite3.c: 113110 us
  parse       sqlite3.c:  74647 us
  typecheck   sqlite3.c:  22960 us
  codegen     sqlite3.c: 524159 us
  peephole    sqlite3.c: 157489 us
  link        null: 5112856 us

RCC -O1:
  preprocess  sqlite3.c: 277541 us
  lex         sqlite3.c: 104738 us
  parse       sqlite3.c:  66350 us
  typecheck   sqlite3.c:  22818 us
  opt(CTFE)   sqlite3.c:  22714 us
  codegen     sqlite3.c: 507451 us
  peephole    sqlite3.c: 157652 us
  link        null: 5018182 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       6121ms |
| RCC -O1   |       6038ms |
| TCC       |         76ms |
| GCC -O0   |        849ms |
| GCC -O2   |       8730ms |
| Clang -O0 |        833ms |
| Clang -O2 |       8858ms |
