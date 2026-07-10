# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           60 |          678 |        738 |
| RCC -O1   |           68 |          645 |        713 |
| TCC       |           54 |          589 |        643 |
| GCC -O0   |          115 |          524 |        639 |
| GCC -O2   |          207 |          315 |        522 |
| Clang -O0 |           91 |          472 |        563 |
| Clang -O2 |          229 |          330 |        559 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    524 us
  lex         bench.c:     74 us
  parse       bench.c:    107 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    144 us
  link        bench_rcc: 102279 us

RCC -O1:
  preprocess  bench.c:    511 us
  lex         bench.c:     74 us
  parse       bench.c:    102 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    152 us
  link        bench_rcc_o1:  90281 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
sysctl.h:794: error: #error Use the SYSCTL_*() macros and (-1) instead!

RCC -O1:
sysctl.h:794: error: #error Use the SYSCTL_*() macros and (-1) instead!
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       127 ms |
| GCC -O0   |      1538 ms |
| GCC -O2   |     11649 ms |
| Clang -O0 |      1057 ms |
| Clang -O2 |     11406 ms |
