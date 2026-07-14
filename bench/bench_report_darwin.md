# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          108 |          858 |        966 |
| RCC -O1   |          118 |          792 |        910 |
| TCC       |           70 |          680 |        750 |
| GCC -O0   |          128 |          613 |        741 |
| GCC -O2   |          152 |          307 |        459 |
| Clang -O0 |          170 |          588 |        758 |
| Clang -O2 |          208 |          341 |        549 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1577 us
  lex         bench.c:     91 us
  parse       bench.c:    176 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    200 us
  link        bench_rcc:  72191 us

RCC -O1:
  preprocess  bench.c:    800 us
  lex         bench.c:     76 us
  parse       bench.c:    104 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     15 us
  codegen     bench.c:    145 us
  link        bench_rcc_o1:  86502 us
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
| TCC       |       188 ms |
| GCC -O0   |      1590 ms |
| GCC -O2   |     17306 ms |
| Clang -O0 |      1693 ms |
| Clang -O2 |     13987 ms |
