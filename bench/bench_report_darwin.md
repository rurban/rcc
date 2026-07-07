# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          184 |          656 |        840 |
| RCC -O1   |           76 |          680 |        756 |
| TCC       |           77 |          577 |        654 |
| GCC -O0   |          104 |          482 |        586 |
| GCC -O2   |          312 |          369 |        681 |
| Clang -O0 |          122 |          622 |        744 |
| Clang -O2 |          258 |          400 |        658 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    916 us
  lex         bench.c:    167 us
  parse       bench.c:    468 us
  typecheck   bench.c:     10 us
  codegen     bench.c:    413 us
  link        bench_rcc:  96297 us

RCC -O1:
  preprocess  bench.c:    542 us
  lex         bench.c:     88 us
  parse       bench.c:    416 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     15 us
  codegen     bench.c:    166 us
  link        bench_rcc_o1:  74016 us
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
| TCC       |       255 ms |
| GCC -O0   |      2237 ms |
| GCC -O2   |     16992 ms |
| Clang -O0 |      1804 ms |
| Clang -O2 |     15141 ms |
