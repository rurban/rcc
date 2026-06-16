# Linux RCC Benchmark Results

_Generated: Juni 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           46 |          644 |        690 |
| RCC -O1   |           51 |          660 |        711 |
| TCC       |            7 |          551 |        558 |
| SLIMCC    |           42 |          561 |        603 |
| KEFIR     |          213 |          653 |        866 |
| KEFIR -O1 |          204 |          342 |        546 |
| CCC       |           43 |          609 |        652 |
| GCC -O0   |           61 |          532 |        593 |
| GCC -O2   |          149 |          198 |        347 |
| Clang -O0 |           86 |          498 |        584 |
| Clang -O2 |          152 |          193 |        345 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    521 us
  lex         bench.c:    113 us
  parse       bench.c:    322 us
  typecheck   bench.c:     19 us
  codegen     bench.c:    947 us
  peephole    bench.c:    245 us
  link        bench_rcc:  41406 us

RCC -O1:
  preprocess  bench.c:    417 us
  lex         bench.c:    157 us
  parse       bench.c:    285 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     24 us
  codegen     bench.c:    984 us
  peephole    bench.c:    303 us
  link        bench_rcc_o1:  45652 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 266952 us
  lex         sqlite3.c: 104099 us
  parse       sqlite3.c: 136700 us
  typecheck   sqlite3.c:   9680 us
  codegen     sqlite3.c: 511997 us
  peephole    sqlite3.c: 236503 us
  link        null: 441348 us

RCC -O1:
  preprocess  sqlite3.c: 238519 us
  lex         sqlite3.c:  86373 us
  parse       sqlite3.c: 146562 us
  typecheck   sqlite3.c:   9105 us
  opt(CTFE)   sqlite3.c:  33244 us
  codegen     sqlite3.c: 536274 us
  peephole    sqlite3.c: 249368 us
  link        null: 470504 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1404 ms |
| RCC -O1   |      1480 ms |
| TCC       |       117 ms |
| SLIMCC    |      1037 ms |
| KEFIR     |     23689 ms |
| KEFIR     |     46038 ms |
| CCC       |     16772 ms |
| GCC -O0   |      5146 ms |
| GCC -O2   |     29392 ms |
| Clang -O0 |      2094 ms |
| Clang -O2 |     28740 ms |
