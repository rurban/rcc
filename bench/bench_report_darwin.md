# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           87 |          595 |        682 |
| RCC -O1   |           55 |          655 |        710 |
| TCC       |           42 |          561 |        603 |
| GCC -O0   |           82 |          442 |        524 |
| GCC -O2   |           96 |          265 |        361 |
| Clang -O0 |           73 |          464 |        537 |
| Clang -O2 |          122 |          304 |        426 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1380 us
  lex         bench.c:    208 us
  parse       bench.c:    259 us
  typecheck   bench.c:     10 us
  codegen     bench.c:    333 us
  link        bench_rcc:  85825 us

RCC -O1:
  preprocess  bench.c:    621 us
  lex         bench.c:     79 us
  parse       bench.c:    169 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    177 us
  link        bench_rcc_o1:  83792 us
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
| TCC       |       224 ms |
| GCC -O0   |      1494 ms |
| GCC -O2   |     14317 ms |
| Clang -O0 |      1269 ms |
| Clang -O2 |     10240 ms |
