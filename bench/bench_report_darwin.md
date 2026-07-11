# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           56 |          624 |        680 |
| RCC -O1   |           50 |          581 |        631 |
| TCC       |           36 |          525 |        561 |
| GCC -O0   |           72 |          435 |        507 |
| GCC -O2   |          113 |          263 |        376 |
| Clang -O0 |           56 |          436 |        492 |
| Clang -O2 |           92 |          262 |        354 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    539 us
  lex         bench.c:     77 us
  parse       bench.c:    120 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    164 us
  link        bench_rcc:  45985 us

RCC -O1:
  preprocess  bench.c:    516 us
  lex         bench.c:     81 us
  parse       bench.c:    118 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    161 us
  link        bench_rcc_o1:  50735 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |        87 ms |
| GCC -O0   |       888 ms |
| GCC -O2   |      8740 ms |
| Clang -O0 |       930 ms |
| Clang -O2 |      8702 ms |
