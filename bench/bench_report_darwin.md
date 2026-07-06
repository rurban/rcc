# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          105 |          691 |        796 |
| RCC -O1   |           98 |          681 |        779 |
| TCC       |           63 |          561 |        624 |
| GCC -O0   |          105 |          470 |        575 |
| GCC -O2   |          127 |          288 |        415 |
| Clang -O0 |           81 |          476 |        557 |
| Clang -O2 |          113 |          296 |        409 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1508 us
  lex         bench.c:     88 us
  parse       bench.c:    102 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    191 us
  link        bench_rcc:  58337 us

RCC -O1:
  preprocess  bench.c:   1724 us
  lex         bench.c:     79 us
  parse       bench.c:    119 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     18 us
  codegen     bench.c:    154 us
  link        bench_rcc_o1:  61374 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 2202748 us
  lex         sqlite3.c: 114004 us
  parse       sqlite3.c: 122788 us
  typecheck   sqlite3.c:  24968 us
  codegen     sqlite3.c: 3090995 us

RCC -O1:
  preprocess  sqlite3.c: 1359185 us
  lex         sqlite3.c:  95199 us
  parse       sqlite3.c:  64113 us
  typecheck   sqlite3.c:  31362 us
  opt(CTFE)   sqlite3.c:  42856 us
  codegen     sqlite3.c: 2858201 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      4774 ms |
| RCC -O1   |      4985 ms |
| TCC       |       102 ms |
| GCC -O0   |      1164 ms |
| GCC -O2   |     11820 ms |
| Clang -O0 |      1176 ms |
| Clang -O2 |     10452 ms |
