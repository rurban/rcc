# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           56 |          544 |        600 |
| RCC -O1   |           57 |          586 |        643 |
| TCC       |           26 |            1 |         27 |
| GCC -O0   |          113 |          400 |        513 |
| GCC -O2   |          110 |          237 |        347 |
| Clang -O0 |           43 |          409 |        452 |
| Clang -O2 |           70 |          246 |        316 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1091 us
  lex         bench.c:    112 us
  parse       bench.c:    126 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   3216 us
  peephole    bench.c:    264 us
  link        bench_rcc: 158584 us

RCC -O1:
  preprocess  bench.c:    464 us
  lex         bench.c:     93 us
  parse       bench.c:     47 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1519 us
  peephole    bench.c:    217 us
  link        bench_rcc_o1:  51928 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 306581 us
  lex         sqlite3.c:  90875 us
sqlite3.c:1: [0m[1;31merror:[0m undeclared variable
 FILENAME_MAX) ? (void)0 : abort());
 [1;31m^~~~~~~~~~~~[0m

RCC -O1:
  preprocess  sqlite3.c: 247265 us
  lex         sqlite3.c:  89319 us
sqlite3.c:1: [0m[1;31merror:[0m undeclared variable
 FILENAME_MAX) ? (void)0 : abort());
 [1;31m^~~~~~~~~~~~[0m
```
