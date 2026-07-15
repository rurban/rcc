# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           66 |          672 |        738 |
| RCC -O1   |          143 |          711 |        854 |
| TCC       |            9 |          638 |        647 |
| SLIMCC    |           61 |          894 |        955 |
| KEFIR     |          290 |          795 |       1085 |
| KEFIR -O1 |          229 |          625 |        854 |
| CCC       |           55 |          614 |        669 |
| GCC -O0   |          103 |          603 |        706 |
| GCC -O2   |          193 |          233 |        426 |
| Clang -O0 |          117 |          704 |        821 |
| Clang -O2 |          194 |          242 |        436 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   3769 us
  lex         bench.c:    373 us
  parse       bench.c:    486 us
  typecheck   bench.c:      9 us
  codegen     bench.c:    560 us
  link        bench_rcc:  60118 us

RCC -O1:
  preprocess  bench.c:   7596 us
  lex         bench.c:    724 us
  parse       bench.c:    888 us
  typecheck   bench.c:     23 us
  opt(CTFE)   bench.c:     29 us
  codegen     bench.c:   1260 us
  link        bench_rcc_o1:  55032 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 284996 us
  lex         sqlite3.c: 162593 us
  parse       sqlite3.c: 343961 us
  typecheck   sqlite3.c:  24900 us
  codegen     sqlite3.c: 6271165 us

RCC -O1:
  preprocess  sqlite3.c: 269839 us
  lex         sqlite3.c: 120961 us
  parse       sqlite3.c: 175976 us
  typecheck   sqlite3.c:  15123 us
  opt(CTFE)   sqlite3.c:  31911 us
  codegen     sqlite3.c: 6222122 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      7588 ms |
| RCC -O1   |      7805 ms |
| TCC       |       146 ms |
| SLIMCC    |      1700 ms |
| KEFIR     |     36788 ms |
| KEFIR -O1 |     49705 ms |
| CCC       |     33447 ms |
| GCC -O0   |     22443 ms |
| GCC -O2   |    151552 ms |
| Clang -O0 |      5580 ms |
| Clang -O2 |     77220 ms |
