# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           88 |          722 |        810 |
| RCC -O1   |           99 |          775 |        874 |
| TCC       |          110 |          629 |        739 |
| GCC -O0   |          148 |          565 |        713 |
| GCC -O2   |          248 |          350 |        598 |
| Clang -O0 |          153 |          598 |        751 |
| Clang -O2 |          173 |          363 |        536 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    631 us
  lex         bench.c:     83 us
  parse       bench.c:    131 us
  typecheck   bench.c:      3 us
  codegen     bench.c:    164 us
  link        bench_rcc:  61720 us

RCC -O1:
  preprocess  bench.c:    554 us
  lex         bench.c:    183 us
  parse       bench.c:    112 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     15 us
  codegen     bench.c:    137 us
  link        bench_rcc_o1:  72133 us
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
| TCC       |       156 ms |
| GCC -O0   |      1896 ms |
| GCC -O2   |     13741 ms |
| Clang -O0 |      1492 ms |
| Clang -O2 |     11994 ms |
