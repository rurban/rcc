# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           58 |          659 |        717 |
| RCC -O1   |           67 |          601 |        668 |
| TCC       |           37 |          536 |        573 |
| GCC -O0   |           68 |          484 |        552 |
| GCC -O2   |          138 |          286 |        424 |
| Clang -O0 |           63 |          446 |        509 |
| Clang -O2 |          107 |          269 |        376 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    840 us
  lex         bench.c:     83 us
  parse       bench.c:    178 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    189 us
  link        bench_rcc:  65315 us

RCC -O1:
  preprocess  bench.c:    548 us
  lex         bench.c:     80 us
  parse       bench.c:    127 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    151 us
  link        bench_rcc_o1:  63980 us
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
| TCC       |       117 ms |
| GCC -O0   |       993 ms |
| GCC -O2   |     10284 ms |
| Clang -O0 |      1004 ms |
| Clang -O2 |     10361 ms |
