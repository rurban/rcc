# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          625 |        678 |
| RCC -O1   |           67 |          648 |        715 |
| TCC       |            9 |          622 |        631 |
| SLIMCC    |           55 |          670 |        725 |
| KEFIR     |          237 |          691 |        928 |
| KEFIR -O1 |          197 |          505 |        702 |
| CCC       |           51 |          679 |        730 |
| GCC -O0   |           97 |          605 |        702 |
| GCC -O2   |          199 |          215 |        414 |
| Clang -O0 |          105 |          659 |        764 |
| Clang -O2 |          143 |          239 |        382 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   6533 us
  lex         bench.c:    764 us
  parse       bench.c:    767 us
  typecheck   bench.c:     23 us
  codegen     bench.c:    687 us
  link        bench_rcc:  49294 us

RCC -O1:
  preprocess  bench.c:   5631 us
  lex         bench.c:    537 us
  parse       bench.c:    779 us
  typecheck   bench.c:     18 us
  opt(CTFE)   bench.c:     21 us
  codegen     bench.c:    645 us
  link        bench_rcc_o1: 123663 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 437776 us
  lex         sqlite3.c: 110246 us
  parse       sqlite3.c: 160445 us
  typecheck   sqlite3.c:  13954 us
  codegen     sqlite3.c: 147689 us

RCC -O1:
  preprocess  sqlite3.c: 165325 us
  lex         sqlite3.c: 109644 us
  parse       sqlite3.c: 173999 us
  typecheck   sqlite3.c:  14911 us
  opt(CTFE)   sqlite3.c:  31852 us
  codegen     sqlite3.c: 123971 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       666 ms |
| RCC -O1   |       933 ms |
| TCC       |       117 ms |
| SLIMCC    |      1333 ms |
| KEFIR     |     27379 ms |
| KEFIR -O1 |     30892 ms |
| CCC       |     20158 ms |
| GCC -O0   |     12470 ms |
| GCC -O2   |     80277 ms |
| Clang -O0 |      3668 ms |
| Clang -O2 |     45004 ms |
