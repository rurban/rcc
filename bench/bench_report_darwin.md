# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           48 |          673 |        721 |
| RCC -O1   |           51 |          611 |        662 |
| TCC       |           40 |          553 |        593 |
| GCC -O0   |           65 |          443 |        508 |
| GCC -O2   |          126 |          282 |        408 |
| Clang -O0 |           53 |          467 |        520 |
| Clang -O2 |          113 |          281 |        394 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    506 us
  lex         bench.c:     85 us
  parse       bench.c:    113 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    138 us
  link        bench_rcc:  49169 us

RCC -O1:
  preprocess  bench.c:    447 us
  lex         bench.c:     74 us
  parse       bench.c:    123 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:    147 us
  link        bench_rcc_o1:  44748 us
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
| TCC       |       101 ms |
| GCC -O0   |      1051 ms |
| GCC -O2   |     10160 ms |
| Clang -O0 |      1046 ms |
| Clang -O2 |      9842 ms |
