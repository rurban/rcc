# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           43 |          609 |        652 |
| RCC -O1   |           47 |          607 |        654 |
| TCC       |           12 |          563 |        575 |
| SLIMCC    |           47 |          629 |        676 |
| KEFIR     |          210 |          672 |        882 |
| KEFIR -O1 |          189 |          489 |        678 |
| CCC       |           51 |          564 |        615 |
| GCC -O0   |           68 |          570 |        638 |
| GCC -O2   |          180 |          211 |        391 |
| Clang -O0 |          112 |          632 |        744 |
| Clang -O2 |          145 |          233 |        378 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   6337 us
  lex         bench.c:    723 us
  parse       bench.c:   1298 us
  typecheck   bench.c:     31 us
  codegen     bench.c:    766 us
  link        bench_rcc:  38045 us

RCC -O1:
  preprocess  bench.c:   3497 us
  lex         bench.c:    330 us
  parse       bench.c:    498 us
  typecheck   bench.c:     10 us
  opt(CTFE)   bench.c:     22 us
  codegen     bench.c:    433 us
  link        bench_rcc_o1:  40287 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 150842 us
  lex         sqlite3.c:  79119 us
  parse       sqlite3.c: 146511 us
  typecheck   sqlite3.c:  13544 us
  codegen     sqlite3.c: 114909 us

RCC -O1:
  preprocess  sqlite3.c: 140717 us
  lex         sqlite3.c:  75364 us
  parse       sqlite3.c: 143244 us
  typecheck   sqlite3.c:  13836 us
  opt(CTFE)   sqlite3.c:  31539 us
  codegen     sqlite3.c: 122240 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       542 ms |
| RCC -O1   |       566 ms |
| TCC       |       121 ms |
| SLIMCC    |      1255 ms |
| KEFIR     |     23246 ms |
| KEFIR -O1 |     26026 ms |
| CCC       |     17399 ms |
| GCC -O0   |     10418 ms |
| GCC -O2   |     67898 ms |
| Clang -O0 |      3146 ms |
| Clang -O2 |     40258 ms |
