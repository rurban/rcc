# Linux RCC Benchmark Results

_Generated: Juni 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           34 |          817 |        851 |
| RCC -O1   |           34 |          780 |        814 |
| TCC       |            7 |          612 |        619 |
| SLIMCC    |           47 |          613 |        660 |
| KEFIR     |          250 |          794 |       1044 |
| KEFIR -O1 |          278 |          376 |        654 |
| CCC       |           54 |          631 |        685 |
| GCC -O0   |           74 |          653 |        727 |
| GCC -O2   |          193 |          218 |        411 |
| Clang -O0 |          101 |          603 |        704 |
| Clang -O2 |          158 |          221 |        379 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    652 us
  lex         bench.c:    544 us
  parse       bench.c:    688 us
  typecheck   bench.c:     12 us
  codegen     bench.c:    603 us
  peephole    bench.c:    253 us
  link        bench_rcc:  32316 us

RCC -O1:
  preprocess  bench.c:    693 us
  lex         bench.c:    380 us
  parse       bench.c:    427 us
  typecheck   bench.c:      9 us
  opt(CTFE)   bench.c:     56 us
  codegen     bench.c:    646 us
  peephole    bench.c:    519 us
  link        bench_rcc_o1:  31444 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 259249 us
  lex         sqlite3.c: 207131 us
  parse       sqlite3.c: 162778 us
  typecheck   sqlite3.c:  11626 us
  codegen     sqlite3.c: 1151206 us
  peephole    sqlite3.c: 6967840 us

RCC -O1:
  preprocess  sqlite3.c: 362527 us
  lex         sqlite3.c: 223889 us
  parse       sqlite3.c: 163598 us
  typecheck   sqlite3.c:  13423 us
  opt(CTFE)   sqlite3.c:  46264 us
  codegen     sqlite3.c: 1119292 us
  peephole    sqlite3.c: 6533280 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      7004 ms |
| RCC -O1   |      7098 ms |
| TCC       |       120 ms |
| SLIMCC    |       459 ms |
| KEFIR     |     28520 ms |
| KEFIR     |     55904 ms |
| CCC       |     18374 ms |
| GCC -O0   |      5884 ms |
| GCC -O2   |     35273 ms |
| Clang -O0 |      2909 ms |
| Clang -O2 |     33225 ms |
