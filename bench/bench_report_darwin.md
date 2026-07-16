# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           52 |          593 |        645 |
| RCC -O1   |           61 |          624 |        685 |
| TCC       |           34 |          519 |        553 |
| GCC -O0   |           72 |          436 |        508 |
| GCC -O2   |          102 |          264 |        366 |
| Clang -O0 |           59 |          459 |        518 |
| Clang -O2 |           86 |          278 |        364 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    543 us
  lex         bench.c:     90 us
  parse       bench.c:    123 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    122 us
  link        bench_rcc:  46431 us

RCC -O1:
  preprocess  bench.c:    464 us
  lex         bench.c:     84 us
  parse       bench.c:    160 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     30 us
  codegen     bench.c:    144 us
  link        bench_rcc_o1:  46826 us
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
| TCC       |       140 ms |
| GCC -O0   |      1186 ms |
| GCC -O2   |     13363 ms |
| Clang -O0 |      1384 ms |
| Clang -O2 |     11787 ms |
