# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           88 |          610 |        698 |
| RCC -O1   |           77 |          581 |        658 |
| TCC       |           62 |          535 |        597 |
| GCC -O0   |           78 |          455 |        533 |
| GCC -O2   |          104 |          266 |        370 |
| Clang -O0 |           72 |          457 |        529 |
| Clang -O2 |          106 |          268 |        374 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1058 us
  lex         bench.c:     82 us
  parse       bench.c:    107 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1344 us
  peephole    bench.c:    226 us
  link        bench_rcc:  70872 us

RCC -O1:
  preprocess  bench.c:    926 us
  lex         bench.c:     76 us
  parse       bench.c:     90 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:   1741 us
  peephole    bench.c:    243 us
  link        bench_rcc_o1:  72025 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 501660 us
  lex         sqlite3.c: 122089 us
  parse       sqlite3.c: 131562 us
  typecheck   sqlite3.c:  25184 us
  codegen     sqlite3.c: 648587 us
  peephole    sqlite3.c: 169972 us
  link        null: 5271013 us

RCC -O1:
  preprocess  sqlite3.c: 429410 us
  lex         sqlite3.c: 117667 us
  parse       sqlite3.c: 172522 us
  typecheck   sqlite3.c:  15640 us
  opt(CTFE)   sqlite3.c:  19297 us
  codegen     sqlite3.c: 618370 us
  peephole    sqlite3.c: 165038 us
  link        null: 5307871 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      6940 ms |
| RCC -O1   |      7008 ms |
| TCC       |       152 ms |
| GCC -O0   |      1049 ms |
| GCC -O2   |     10040 ms |
| Clang -O0 |       885 ms |
| Clang -O2 |      9371 ms |
