# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           60 |          573 |        633 |
| RCC -O1   |           48 |          624 |        672 |
| TCC       |           34 |          512 |        546 |
| GCC -O0   |           62 |          435 |        497 |
| GCC -O2   |           92 |          264 |        356 |
| Clang -O0 |           54 |          434 |        488 |
| Clang -O2 |           82 |          263 |        345 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    530 us
  lex         bench.c:     66 us
  parse       bench.c:     89 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    131 us
  link        bench_rcc:  49313 us

RCC -O1:
  preprocess  bench.c:    465 us
  lex         bench.c:     65 us
  parse       bench.c:     84 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    122 us
  link        bench_rcc_o1:  45480 us
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
| TCC       |        88 ms |
| GCC -O0   |       890 ms |
| GCC -O2   |      8881 ms |
| Clang -O0 |       889 ms |
| Clang -O2 |      8946 ms |
