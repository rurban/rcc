# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          168 |          809 |        977 |
| RCC -O1   |          210 |          832 |       1042 |
| TCC       |          104 |          764 |        868 |
| GCC -O0   |          217 |          654 |        871 |
| GCC -O2   |          230 |          351 |        581 |
| Clang -O0 |          111 |          616 |        727 |
| Clang -O2 |          162 |          370 |        532 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2328 us
  lex         bench.c:    105 us
  parse       bench.c:    107 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   8269 us
  peephole    bench.c:    526 us
  link        bench_rcc: 133537 us

RCC -O1:
  preprocess  bench.c:   1267 us
  lex         bench.c:    104 us
  parse       bench.c:    132 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:   6364 us
  peephole    bench.c:    515 us
  link        bench_rcc_o1: 116003 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1198062 us
  lex         sqlite3.c: 294282 us
  parse       sqlite3.c: 227717 us
  typecheck   sqlite3.c:  48545 us
  codegen     sqlite3.c: 1827611 us
  peephole    sqlite3.c: 320402 us
  link        null: 7805818 us

RCC -O1:
  preprocess  sqlite3.c: 1124991 us
  lex         sqlite3.c: 206734 us
  parse       sqlite3.c: 153278 us
  typecheck   sqlite3.c:  32614 us
  opt(CTFE)   sqlite3.c:  39313 us
  codegen     sqlite3.c: 1681495 us
  peephole    sqlite3.c: 311540 us
  link        null: 7513865 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |     10017 ms |
| RCC -O1   |     11292 ms |
| TCC       |       267 ms |
| GCC -O0   |      1838 ms |
| GCC -O2   |     17419 ms |
| Clang -O0 |      2052 ms |
| Clang -O2 |     16514 ms |
