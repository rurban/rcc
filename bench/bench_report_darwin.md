# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          133 |          661 |        794 |
| RCC -O1   |           74 |          622 |        696 |
| TCC       |           51 |          628 |        679 |
| GCC -O0   |          104 |          533 |        637 |
| GCC -O2   |          234 |          331 |        565 |
| Clang -O0 |          129 |          566 |        695 |
| Clang -O2 |          176 |          354 |        530 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1139 us
  lex         bench.c:     79 us
  parse       bench.c:     88 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    141 us
  link        bench_rcc:  48323 us

RCC -O1:
  preprocess  bench.c:   1263 us
  lex         bench.c:     93 us
  parse       bench.c:    105 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    154 us
  link        bench_rcc_o1:  47983 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 2226854 us
  lex         sqlite3.c: 106955 us
  parse       sqlite3.c:  96684 us
  typecheck   sqlite3.c:  17578 us
  codegen     sqlite3.c: 2844141 us

RCC -O1:
  preprocess  sqlite3.c: 1294748 us
  lex         sqlite3.c: 105404 us
  parse       sqlite3.c:  94733 us
  typecheck   sqlite3.c:  26408 us
  opt(CTFE)   sqlite3.c:  30753 us
  codegen     sqlite3.c: 3690360 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      6737 ms |
| RCC -O1   |      6036 ms |
| TCC       |       159 ms |
| GCC -O0   |      2023 ms |
| GCC -O2   |     12974 ms |
| Clang -O0 |      1144 ms |
| Clang -O2 |     11905 ms |
