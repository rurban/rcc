# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           90 |          693 |        783 |
| RCC -O1   |           85 |          664 |        749 |
| TCC       |           46 |          573 |        619 |
| GCC -O0   |          106 |          509 |        615 |
| GCC -O2   |          149 |          340 |        489 |
| Clang -O0 |           95 |          566 |        661 |
| Clang -O2 |          229 |          371 |        600 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1320 us
  lex         bench.c:     78 us
  parse       bench.c:    135 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    185 us
  link        bench_rcc:  92388 us

RCC -O1:
  preprocess  bench.c:    545 us
  lex         bench.c:    144 us
  parse       bench.c:    137 us
  typecheck   bench.c:     33 us
  opt(CTFE)   bench.c:     98 us
  codegen     bench.c:    202 us
  link        bench_rcc_o1:  94819 us
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
| TCC       |       184 ms |
| GCC -O0   |      1879 ms |
| GCC -O2   |     20253 ms |
| Clang -O0 |      2313 ms |
| Clang -O2 |     16184 ms |
