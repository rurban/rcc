# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          123 |          547 |        670 |
| RCC -O1   |           54 |          545 |        599 |
| TCC       |           24 |            1 |         25 |
| GCC -O0   |           60 |          405 |        465 |
| GCC -O2   |          105 |          239 |        344 |
| Clang -O0 |           55 |          409 |        464 |
| Clang -O2 |           82 |          258 |        340 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   3107 us
  lex         bench.c:    508 us
  parse       bench.c:    516 us
  typecheck   bench.c:     21 us
  codegen     bench.c:   6762 us
  peephole    bench.c:    809 us
  link        bench_rcc:  84480 us

RCC -O1:
  preprocess  bench.c:    545 us
  lex         bench.c:     92 us
  parse       bench.c:     48 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1512 us
  peephole    bench.c:    212 us
  link        bench_rcc_o1:  51893 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 251309 us
  lex         sqlite3.c:  78163 us
  parse       sqlite3.c: 1018884 us
  typecheck   sqlite3.c:  14942 us
  codegen     sqlite3.c: 755356 us
  peephole    sqlite3.c: 164125 us
  link        null: 4851937 us

RCC -O1:
  preprocess  sqlite3.c: 222614 us
  lex         sqlite3.c:  79961 us
  parse       sqlite3.c: 1053188 us
  typecheck   sqlite3.c:  12540 us
  opt(CTFE)   sqlite3.c:  15845 us
  codegen     sqlite3.c: 727072 us
  peephole    sqlite3.c: 142787 us
  link        null: 4529762 us
```
