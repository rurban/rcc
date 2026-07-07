# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          125 |          870 |        995 |
| RCC -O1   |          176 |          858 |       1034 |
| TCC       |          101 |          775 |        876 |
| GCC -O0   |          183 |          705 |        888 |
| GCC -O2   |          262 |          372 |        634 |
| Clang -O0 |           94 |          575 |        669 |
| Clang -O2 |          167 |          314 |        481 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2600 us
  lex         bench.c:     97 us
  parse       bench.c:     87 us
  typecheck   bench.c:     34 us
  codegen     bench.c:    147 us
  link        bench_rcc:  59902 us

RCC -O1:
  preprocess  bench.c:   2298 us
  lex         bench.c:     92 us
  parse       bench.c:     88 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    169 us
  link        bench_rcc_o1:  63351 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1279285 us
  lex         sqlite3.c:  97066 us
  parse       sqlite3.c: 102497 us
  typecheck   sqlite3.c:  22343 us
  codegen     sqlite3.c: 3785278 us

RCC -O1:
  preprocess  sqlite3.c: 1032505 us
  lex         sqlite3.c: 121181 us
  parse       sqlite3.c:  94450 us
  typecheck   sqlite3.c:  26785 us
  opt(CTFE)   sqlite3.c:  38578 us
  codegen     sqlite3.c: 4188519 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      5835 ms |
| RCC -O1   |      6129 ms |
| TCC       |       139 ms |
| GCC -O0   |      1415 ms |
| GCC -O2   |     12163 ms |
| Clang -O0 |      1452 ms |
| Clang -O2 |     14757 ms |
