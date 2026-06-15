# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          227 |          742 |        969 |
| RCC -O1   |          185 |          651 |        836 |
| TCC       |           53 |          583 |        636 |
| GCC -O0   |          127 |          490 |        617 |
| GCC -O2   |          136 |          288 |        424 |
| Clang -O0 |           75 |          477 |        552 |
| Clang -O2 |          139 |          289 |        428 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    804 us
  lex         bench.c:    101 us
  parse       bench.c:     88 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1655 us
  peephole    bench.c:    229 us
  link        bench_rcc:  66634 us

RCC -O1:
  preprocess  bench.c:    787 us
  lex         bench.c:    105 us
  parse       bench.c:     91 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:   1611 us
  peephole    bench.c:    274 us
  link        bench_rcc_o1:  72335 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 388464 us
  lex         sqlite3.c: 160420 us
  parse       sqlite3.c: 197698 us
  typecheck   sqlite3.c:  27257 us
  codegen     sqlite3.c: 761758 us
  peephole    sqlite3.c: 201215 us
  link        null: 5496429 us

RCC -O1:
  preprocess  sqlite3.c: 433512 us
  lex         sqlite3.c: 136345 us
  parse       sqlite3.c:  90389 us
  typecheck   sqlite3.c:  12921 us
  opt(CTFE)   sqlite3.c:  19435 us
  codegen     sqlite3.c: 531761 us
  peephole    sqlite3.c: 158899 us
  link        null: 6458672 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      9464 ms |
| RCC -O1   |      8135 ms |
| TCC       |       110 ms |
| GCC -O0   |      1097 ms |
| GCC -O2   |     11479 ms |
| Clang -O0 |      1465 ms |
| Clang -O2 |     11547 ms |
