# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           44 |          612 |        656 |
| RCC -O1   |           42 |          605 |        647 |
| RCC -O2   |           45 |          578 |        623 |
| TCC       |           13 |          569 |        582 |
| SLIMCC    |           51 |          639 |        690 |
| XCC       |           23 |          360 |        383 |
| KEFIR     |          202 |          681 |        883 |
| KEFIR -O1 |          192 |          508 |        700 |
| CCC       |           69 |          567 |        636 |
| GCC -O0   |           69 |          569 |        638 |
| GCC -O2   |          196 |          217 |        413 |
| Clang -O0 |          129 |          631 |        760 |
| Clang -O2 |          144 |          237 |        381 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   5732 us
  parse       bench.c:    853 us
  typecheck   bench.c:     10 us
  codegen     bench.c:    502 us
  link        bench_rcc:  35051 us

RCC -O1:
  preprocess  bench.c:   4179 us
  parse       bench.c:    507 us
  typecheck   bench.c:     16 us
  opt         bench.c:     31 us
  codegen     bench.c:    370 us
  link        bench_rcc_o1:  35317 us

RCC -O2:
  preprocess  bench.c:   4347 us
  parse       bench.c:    467 us
  typecheck   bench.c:      9 us
  opt         bench.c:     30 us
  codegen     bench.c:    404 us
  link        bench_rcc_o2:  37511 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 283038 us
  parse       sqlite3.c: 163089 us
  typecheck   sqlite3.c:  14089 us
  codegen     sqlite3.c: 124436 us

RCC -O1:
  preprocess  sqlite3.c: 255795 us
  parse       sqlite3.c: 155030 us
  typecheck   sqlite3.c:  13822 us
  opt         sqlite3.c:  43271 us
  codegen     sqlite3.c: 116131 us

RCC -O2:
  preprocess  sqlite3.c: 259123 us
  parse       sqlite3.c: 156449 us
  typecheck   sqlite3.c:  13826 us
  opt         sqlite3.c: 198833 us
  codegen     sqlite3.c: 119922 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       945 ms |
| RCC -O1   |       951 ms |
| RCC -O2   |      1104 ms |
| TCC       |       130 ms |
| SLIMCC    |      1289 ms |
| KEFIR     |     23510 ms |
| KEFIR -O1 |     26727 ms |
| CCC       |     17621 ms |
| GCC -O0   |     10531 ms |
| GCC -O2   |     66307 ms |
| Clang -O0 |      2977 ms |
| Clang -O2 |     37139 ms |
