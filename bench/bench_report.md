# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           48 |          612 |        660 |
| RCC -O1   |           54 |          595 |        649 |
| TCC       |            9 |          600 |        609 |
| SLIMCC    |           63 |          674 |        737 |
| KEFIR     |          245 |          705 |        950 |
| KEFIR -O1 |          212 |          508 |        720 |
| CCC       |           73 |          630 |        703 |
| GCC -O0   |          113 |          593 |        706 |
| GCC -O2   |          214 |          222 |        436 |
| Clang -O0 |          110 |          657 |        767 |
| Clang -O2 |          163 |          240 |        403 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    401 us
  lex         bench.c:    204 us
  parse       bench.c:    376 us
  typecheck   bench.c:      8 us
  codegen     bench.c:    555 us
  link        bench_rcc:  81078 us

RCC -O1:
  preprocess  bench.c:    774 us
  lex         bench.c:    258 us
  parse       bench.c:    459 us
  typecheck   bench.c:     10 us
  opt(CTFE)   bench.c:     27 us
  codegen     bench.c:    930 us
  link        bench_rcc_o1:  48656 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 280372 us
  lex         sqlite3.c: 147314 us
  parse       sqlite3.c: 199023 us
  typecheck   sqlite3.c:  18912 us
  codegen     sqlite3.c: 6467615 us

RCC -O1:
  preprocess  sqlite3.c: 237503 us
  lex         sqlite3.c: 139695 us
  parse       sqlite3.c: 178272 us
  typecheck   sqlite3.c:  21612 us
  opt(CTFE)   sqlite3.c:  46224 us
  codegen     sqlite3.c: 6585384 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      7408 ms |
| RCC -O1   |      7085 ms |
| TCC       |       148 ms |
| SLIMCC    |      1703 ms |
| KEFIR     |     28442 ms |
| KEFIR -O1 |     29555 ms |
| CCC       |     23515 ms |
| GCC -O0   |     12349 ms |
| GCC -O2   |    104596 ms |
| Clang -O0 |      6051 ms |
| Clang -O2 |     71466 ms |
