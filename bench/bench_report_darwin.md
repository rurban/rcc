# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           57 |          593 |        650 |
| RCC -O1   |           60 |          648 |        708 |
| TCC       |           36 |          531 |        567 |
| GCC -O0   |           68 |          451 |        519 |
| GCC -O2   |          113 |          270 |        383 |
| Clang -O0 |           57 |          458 |        515 |
| Clang -O2 |           90 |          272 |        362 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    569 us
  lex         bench.c:     73 us
  parse       bench.c:    136 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    164 us
  link        bench_rcc:  54948 us

RCC -O1:
  preprocess  bench.c:    521 us
  lex         bench.c:     75 us
  parse       bench.c:    119 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    132 us
  link        bench_rcc_o1:  49424 us
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
| TCC       |        91 ms |
| GCC -O0   |       947 ms |
| GCC -O2   |      9620 ms |
| Clang -O0 |       964 ms |
| Clang -O2 |      9407 ms |
