# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           83 |          583 |        666 |
| RCC -O1   |           86 |          591 |        677 |
| TCC       |           35 |          515 |        550 |
| GCC -O0   |           75 |          434 |        509 |
| GCC -O2   |           98 |          264 |        362 |
| Clang -O0 |           58 |          434 |        492 |
| Clang -O2 |           83 |          263 |        346 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    992 us
  lex         bench.c:     71 us
  parse       bench.c:     72 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1227 us
  peephole    bench.c:    291 us (est, 778 calls)
  link        bench_rcc:  56397 us

RCC -O1:
  preprocess  bench.c:   1399 us
  lex         bench.c:     75 us
  parse       bench.c:     85 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:   1422 us
  peephole    bench.c:    290 us (est, 775 calls)
  link        bench_rcc_o1:  61911 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1022856 us
  lex         sqlite3.c:  77870 us
  parse       sqlite3.c:  72454 us
  typecheck   sqlite3.c:  21279 us
  codegen     sqlite3.c: 487274 us
  peephole    sqlite3.c: 203934 us (est, 543824 calls)
  link        null: 4910610 us

RCC -O1:
  preprocess  sqlite3.c: 764841 us
  lex         sqlite3.c:  82697 us
  parse       sqlite3.c:  77253 us
  typecheck   sqlite3.c:  19825 us
  opt(CTFE)   sqlite3.c:  22362 us
  codegen     sqlite3.c: 558281 us
  peephole    sqlite3.c: 202702 us (est, 540540 calls)
  link        null: 5031769 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      6562 ms |
| RCC -O1   |      6382 ms |
| TCC       |        73 ms |
| GCC -O0   |       815 ms |
| GCC -O2   |      8485 ms |
| Clang -O0 |       823 ms |
| Clang -O2 |      8325 ms |
