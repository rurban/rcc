# Linux RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          611 |        664 |
| RCC -O1   |           65 |          590 |        655 |
| TCC       |            9 |          566 |        575 |
| SLIMCC    |           50 |          636 |        686 |
| KEFIR     |          210 |          672 |        882 |
| KEFIR -O1 |          200 |          505 |        705 |
| GCC -O0   |           80 |          576 |        656 |
| GCC -O2   |          189 |          215 |        404 |
| Clang -O0 |          102 |          633 |        735 |
| Clang -O2 |          153 |          238 |        391 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    392 us
  lex         bench.c:    214 us
  parse       bench.c:    335 us
  typecheck   bench.c:      9 us
  codegen     bench.c:    746 us
  peephole    bench.c:    298 us
  link        bench_rcc:  48526 us

RCC -O1:
  preprocess  bench.c:    394 us
  lex         bench.c:    181 us
  parse       bench.c:    323 us
  typecheck   bench.c:     10 us
  opt(CTFE)   bench.c:     28 us
  codegen     bench.c:   1017 us
  peephole    bench.c:    323 us
  link        bench_rcc_o1:  50042 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 222127 us
  lex         sqlite3.c: 145498 us
  parse       sqlite3.c: 162468 us
  typecheck   sqlite3.c:  13026 us
  codegen     sqlite3.c: 360249 us
  peephole    sqlite3.c: 155523 us
  link        null: 123047 us

RCC -O1:
  preprocess  sqlite3.c: 227720 us
  lex         sqlite3.c: 149847 us
  parse       sqlite3.c: 148987 us
  typecheck   sqlite3.c:  13065 us
  opt(CTFE)   sqlite3.c:  30191 us
  codegen     sqlite3.c: 316392 us
  peephole    sqlite3.c: 143759 us
  link        null: 1650559 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       1238ms |
| RCC -O1   |       2718ms |
| TCC       |        123ms |
| SLIMCC    |        471ms |
| GCC -O0   |      10445ms |
| GCC -O2   |      77893ms |
| Clang -O0 |       4317ms |
| Clang -O2 |      39639ms |
