# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           88 |          852 |        940 |
| RCC -O1   |          116 |          762 |        878 |
| TCC       |           77 |          752 |        829 |
| GCC -O0   |          155 |          620 |        775 |
| GCC -O2   |          198 |          323 |        521 |
| Clang -O0 |           73 |          537 |        610 |
| Clang -O2 |          160 |          387 |        547 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    675 us
  lex         bench.c:     92 us
  parse       bench.c:    130 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    163 us
  link        bench_rcc:  79704 us

RCC -O1:
  preprocess  bench.c:    571 us
  lex         bench.c:     92 us
  parse       bench.c:    143 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     16 us
  codegen     bench.c:    161 us
  link        bench_rcc_o1:  80097 us
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
| TCC       |       155 ms |
| GCC -O0   |      1916 ms |
| GCC -O2   |     14517 ms |
| Clang -O0 |      1669 ms |
| Clang -O2 |     20540 ms |
