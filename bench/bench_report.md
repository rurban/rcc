# Linux RCC Benchmark Results

_Generated: Juni 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           44 |          624 |        668 |
| RCC -O1   |           48 |          619 |        667 |
| TCC       |            6 |          526 |        532 |
| SLIMCC    |           40 |          536 |        576 |
| KEFIR     |          190 |          616 |        806 |
| KEFIR -O1 |          198 |          319 |        517 |
| CCC       |           37 |          650 |        687 |
| GCC -O0   |           63 |          505 |        568 |
| GCC -O2   |          139 |          196 |        335 |
| Clang -O0 |           90 |          483 |        573 |
| Clang -O2 |          144 |          186 |        330 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    483 us
  lex         bench.c:    184 us
  parse       bench.c:    314 us
  typecheck   bench.c:     24 us
  codegen     bench.c:    841 us
  peephole    bench.c:    194 us
  link        bench_rcc:  46245 us

RCC -O1:
  preprocess  bench.c:    596 us
  lex         bench.c:    250 us
  parse       bench.c:    452 us
  typecheck   bench.c:      8 us
  opt(CTFE)   bench.c:     44 us
  codegen     bench.c:   1288 us
  peephole    bench.c:    402 us
  link        bench_rcc_o1:  39743 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 229705 us
  lex         sqlite3.c: 121610 us
  parse       sqlite3.c: 127986 us
  typecheck   sqlite3.c:  10116 us
  codegen     sqlite3.c: 396818 us
  peephole    sqlite3.c: 176181 us
  link        null: 432070 us

RCC -O1:
  preprocess  sqlite3.c: 224282 us
  lex         sqlite3.c: 127006 us
  parse       sqlite3.c: 135157 us
  typecheck   sqlite3.c:  10610 us
  opt(CTFE)   sqlite3.c:  30598 us
  codegen     sqlite3.c: 404766 us
  peephole    sqlite3.c: 182472 us
  link        null: 449215 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1642 ms |
| RCC -O1   |      1671 ms |
| TCC       |       115 ms |
| SLIMCC    |      1025 ms |
| KEFIR     |     22076 ms |
| KEFIR     |     41921 ms |
| CCC       |     16051 ms |
| GCC -O0   |      5074 ms |
| GCC -O2   |     30466 ms |
| Clang -O0 |      2226 ms |
| Clang -O2 |     27090 ms |
