# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          112 |          680 |        792 |
| RCC -O1   |           96 |          726 |        822 |
| TCC       |           63 |          599 |        662 |
| GCC -O0   |          116 |          500 |        616 |
| GCC -O2   |          145 |          293 |        438 |
| Clang -O0 |           79 |          485 |        564 |
| Clang -O2 |          145 |          293 |        438 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1280 us
  lex         bench.c:    104 us
  parse       bench.c:    163 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    159 us
  link        bench_rcc:  79248 us

RCC -O1:
  preprocess  bench.c:    657 us
  lex         bench.c:     77 us
  parse       bench.c:    158 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     18 us
  codegen     bench.c:    172 us
  link        bench_rcc_o1:  75044 us
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
| TCC       |       141 ms |
| GCC -O0   |      1260 ms |
| GCC -O2   |     13832 ms |
| Clang -O0 |      1213 ms |
| Clang -O2 |     12810 ms |
