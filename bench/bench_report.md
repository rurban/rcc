# Linux RCC Benchmark Results

_Generated: Juni 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           80 |          914 |        994 |
| RCC -O1   |           79 |          755 |        834 |
| TCC       |            8 |          663 |        671 |
| SLIMCC    |           58 |          626 |        684 |
| KEFIR     |          316 |          762 |       1078 |
| KEFIR -O1 |          340 |          488 |        828 |
| CCC       |          114 |          959 |       1073 |
| GCC -O0   |           74 |          659 |        733 |
| GCC -O2   |          223 |          235 |        458 |
| Clang -O0 |          194 |          588 |        782 |
| Clang -O2 |          251 |          264 |        515 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    578 us
  lex         bench.c:    267 us
  parse       bench.c:    411 us
  typecheck   bench.c:      8 us
  codegen     bench.c:   1629 us
  peephole    bench.c:    699 us
  link        bench_rcc:  68253 us

RCC -O1:
  preprocess  bench.c:    755 us
  lex         bench.c:    255 us
  parse       bench.c:    405 us
  typecheck   bench.c:      9 us
  opt(CTFE)   bench.c:     45 us
  codegen     bench.c:   1309 us
  peephole    bench.c:    358 us
  link        bench_rcc_o1:  59959 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 353837 us
  lex         sqlite3.c: 191093 us
  parse       sqlite3.c: 166132 us
  typecheck   sqlite3.c:  10149 us
  codegen     sqlite3.c: 588754 us
  peephole    sqlite3.c: 269171 us
  link        null: 732157 us

RCC -O1:
  preprocess  sqlite3.c: 306631 us
  lex         sqlite3.c: 167544 us
  parse       sqlite3.c: 176344 us
  typecheck   sqlite3.c:  12860 us
  opt(CTFE)   sqlite3.c:  42790 us
  codegen     sqlite3.c: 596781 us
  peephole    sqlite3.c: 261335 us
  link        null: 914491 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1762 ms |
| RCC -O1   |      1982 ms |
| TCC       |       139 ms |
| SLIMCC    |       517 ms |
| KEFIR     |     30101 ms |
| KEFIR     |     59747 ms |
| CCC       |     18317 ms |
| GCC -O0   |      5816 ms |
| GCC -O2   |     34685 ms |
| Clang -O0 |      2642 ms |
| Clang -O2 |     34974 ms |
