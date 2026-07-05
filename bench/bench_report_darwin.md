# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           52 |          591 |        643 |
| RCC -O1   |           53 |          648 |        701 |
| TCC       |           39 |          530 |        569 |
| GCC -O0   |           69 |          448 |        517 |
| GCC -O2   |          102 |          273 |        375 |
| Clang -O0 |           57 |          462 |        519 |
| Clang -O2 |           89 |          274 |        363 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    474 us
  lex         bench.c:     66 us
  parse       bench.c:     89 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    127 us
  link        bench_rcc:  47662 us

RCC -O1:
  preprocess  bench.c:    468 us
  lex         bench.c:     70 us
  parse       bench.c:     94 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:    126 us
  link        bench_rcc_o1:  46005 us
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
| TCC       |        93 ms |
| GCC -O0   |       936 ms |
| GCC -O2   |      9179 ms |
| Clang -O0 |       969 ms |
| Clang -O2 |      9116 ms |
