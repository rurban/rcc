# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           59 |          649 |        708 |
| RCC -O1   |           51 |          627 |        678 |
| TCC       |           39 |          681 |        720 |
| GCC -O0   |          205 |          563 |        768 |
| GCC -O2   |          219 |          311 |        530 |
| Clang -O0 |          107 |          469 |        576 |
| Clang -O2 |           91 |          280 |        371 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    594 us
  lex         bench.c:     81 us
  parse       bench.c:    114 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    146 us
  link        bench_rcc:  50435 us

RCC -O1:
  preprocess  bench.c:    491 us
  lex         bench.c:     86 us
  parse       bench.c:    102 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    151 us
  link        bench_rcc_o1:  53284 us
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
| TCC       |       152 ms |
| GCC -O0   |      1217 ms |
| GCC -O2   |     12535 ms |
| Clang -O0 |      1135 ms |
| Clang -O2 |     11547 ms |
