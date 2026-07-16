# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           52 |          620 |        672 |
| RCC -O1   |           47 |          676 |        723 |
| TCC       |            7 |          626 |        633 |
| SLIMCC    |           43 |          663 |        706 |
| KEFIR     |          246 |          749 |        995 |
| KEFIR -O1 |          222 |          522 |        744 |
| CCC       |           39 |          686 |        725 |
| GCC -O0   |           81 |          638 |        719 |
| GCC -O2   |          194 |          218 |        412 |
| Clang -O0 |          116 |          675 |        791 |
| Clang -O2 |          160 |          239 |        399 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2993 us
  lex         bench.c:    317 us
  parse       bench.c:    405 us
  typecheck   bench.c:      8 us
  codegen     bench.c:    490 us
  link        bench_rcc:  29349 us

RCC -O1:
  preprocess  bench.c:   3267 us
  lex         bench.c:    414 us
  parse       bench.c:    390 us
  typecheck   bench.c:      8 us
  opt(CTFE)   bench.c:     17 us
  codegen     bench.c:    495 us
  link        bench_rcc_o1:  39088 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 176325 us
  lex         sqlite3.c: 110302 us
  parse       sqlite3.c: 154992 us
  typecheck   sqlite3.c:  14194 us
  codegen     sqlite3.c: 234696 us

RCC -O1:
  preprocess  sqlite3.c: 171670 us
  lex         sqlite3.c: 102873 us
  parse       sqlite3.c: 155238 us
  typecheck   sqlite3.c:  14279 us
  opt(CTFE)   sqlite3.c:  32290 us
  codegen     sqlite3.c: 230648 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       795 ms |
| RCC -O1   |       763 ms |
| TCC       |       125 ms |
| SLIMCC    |      1438 ms |
| KEFIR     |     26593 ms |
| KEFIR -O1 |     29822 ms |
| CCC       |     23745 ms |
| GCC -O0   |     12376 ms |
| GCC -O2   |     78615 ms |
| Clang -O0 |      3432 ms |
| Clang -O2 |     45976 ms |
