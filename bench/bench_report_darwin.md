# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           56 |          682 |        738 |
| RCC -O1   |           53 |          622 |        675 |
| TCC       |           36 |          550 |        586 |
| GCC -O0   |           66 |          463 |        529 |
| GCC -O2   |          107 |          280 |        387 |
| Clang -O0 |           57 |          467 |        524 |
| Clang -O2 |           98 |          282 |        380 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    485 us
  lex         bench.c:     72 us
  parse       bench.c:    109 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    140 us
  link        bench_rcc:  48546 us

RCC -O1:
  preprocess  bench.c:    446 us
  lex         bench.c:     71 us
  parse       bench.c:     97 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:    135 us
  link        bench_rcc_o1:  48980 us
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
| TCC       |       115 ms |
| GCC -O0   |      1389 ms |
| GCC -O2   |     11871 ms |
| Clang -O0 |      1259 ms |
| Clang -O2 |      9756 ms |
