# Linux RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           59 |          639 |        698 |
| RCC -O1   |           47 |          645 |        692 |
| TCC       |            9 |          608 |        617 |
| SLIMCC    |           47 |          669 |        716 |
| KEFIR     |          206 |          730 |        936 |
| KEFIR -O1 |          285 |          514 |        799 |
| GCC -O0   |           82 |          622 |        704 |
| GCC -O2   |          194 |          226 |        420 |
| Clang -O0 |           93 |          664 |        757 |
| Clang -O2 |          146 |          237 |        383 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    529 us
  lex         bench.c:    218 us
  parse       bench.c:    387 us
  typecheck   bench.c:      9 us
  codegen     bench.c:   1230 us
  peephole    bench.c:    241 us (est, 644 calls)
  link        bench_rcc:  55787 us

RCC -O1:
  preprocess  bench.c:    526 us
  lex         bench.c:    269 us
  parse       bench.c:    357 us
  typecheck   bench.c:      7 us
  opt(CTFE)   bench.c:     24 us
  codegen     bench.c:    772 us
  peephole    bench.c:    240 us (est, 640 calls)
  link        bench_rcc_o1:  52130 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 209863 us
  lex         sqlite3.c:  84860 us
  parse       sqlite3.c: 144056 us
  typecheck   sqlite3.c:  12534 us
  codegen     sqlite3.c: 350540 us
  peephole    sqlite3.c: 160429 us (est, 427812 calls)
  link        null: 1674576 us

RCC -O1:
  preprocess  sqlite3.c: 206629 us
  lex         sqlite3.c:  85138 us
  parse       sqlite3.c: 142179 us
  typecheck   sqlite3.c:  13124 us
  opt(CTFE)   sqlite3.c:  30228 us
  codegen     sqlite3.c: 304051 us
  peephole    sqlite3.c: 159352 us (est, 424941 calls)
  link        null: 1687933 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      2815 ms |
| RCC -O1   |      2802 ms |
| TCC       |       123 ms |
| SLIMCC    |      1343 ms |
| KEFIR     |     26569 ms |
| KEFIR     |     28187 ms |
| GCC -O0   |     11907 ms |
| GCC -O2   |     72929 ms |
| Clang -O0 |      3267 ms |
| Clang -O2 |     40399 ms |
