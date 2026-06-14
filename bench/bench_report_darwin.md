# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           58 |          591 |        649 |
| RCC -O1   |           54 |          591 |        645 |
| TCC       |           24 |            2 |         26 |
| GCC -O0   |           49 |          409 |        458 |
| GCC -O2   |           71 |          243 |        314 |
| Clang -O0 |           51 |          412 |        463 |
| Clang -O2 |           66 |          245 |        311 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    841 us
  lex         bench.c:     91 us
  parse       bench.c:    103 us
  typecheck   bench.c:      3 us
  codegen     bench.c:   1386 us
  peephole    bench.c:    207 us
  link        bench_rcc: 145674 us

RCC -O1:
  preprocess  bench.c:    479 us
  lex         bench.c:     94 us
  parse       bench.c:     48 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1316 us
  peephole    bench.c:    202 us
  link        bench_rcc_o1:  49000 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 262386 us
  lex         sqlite3.c:  81447 us
  parse       sqlite3.c: 1043839 us
  typecheck   sqlite3.c:  13121 us
  codegen     sqlite3.c: 656051 us
  peephole    sqlite3.c: 142012 us
  link        null: 4431612 us

RCC -O1:
  preprocess  sqlite3.c: 241787 us
  lex         sqlite3.c:  80766 us
  parse       sqlite3.c: 1033370 us
  typecheck   sqlite3.c:  14180 us
  opt(CTFE)   sqlite3.c:  16613 us
  codegen     sqlite3.c: 672068 us
  peephole    sqlite3.c: 142162 us
  link        null: 4386249 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       6787ms |
| RCC -O1   |       6947ms |
| TCC       |         87ms |
| GCC -O0   |        771ms |
| GCC -O2   |       7557ms |
| Clang -O0 |        742ms |
| Clang -O2 |       7506ms |
