# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           42 |          580 |        622 |
| RCC -O1   |           38 |          609 |        647 |
| RCC -O2   |           40 |          612 |        652 |
| TCC       |            8 |          569 |        577 |
| SLIMCC    |           47 |          628 |        675 |
| XCC       |           11 |          360 |        371 |
| KEFIR     |          191 |          674 |        865 |
| KEFIR -O1 |          202 |          502 |        704 |
| CCC       |           44 |          564 |        608 |
| GCC -O0   |           82 |          575 |        657 |
| GCC -O2   |          183 |          213 |        396 |
| Clang -O0 |           89 |          619 |        708 |
| Clang -O2 |          134 |          233 |        367 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   5266 us
  parse       bench.c:    583 us
  typecheck   bench.c:     11 us
  codegen     bench.c:    475 us
  link        bench_rcc:  35503 us

RCC -O1:
  preprocess  bench.c:   5758 us
  parse       bench.c:    618 us
  typecheck   bench.c:     13 us
  opt         bench.c:     54 us
  codegen     bench.c:    647 us
  link        bench_rcc_o1:  42092 us

RCC -O2:
  preprocess  bench.c:   5336 us
  parse       bench.c:    544 us
  typecheck   bench.c:      9 us
  opt         bench.c:     36 us
  codegen     bench.c:    468 us
  link        bench_rcc_o2:  33613 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 261358 us
  parse       sqlite3.c: 149794 us
  typecheck   sqlite3.c:  13739 us
  codegen     sqlite3.c: 118217 us

RCC -O1:
  preprocess  sqlite3.c: 250564 us
  parse       sqlite3.c: 150907 us
  typecheck   sqlite3.c:  13969 us
  opt         sqlite3.c:  41436 us
  codegen     sqlite3.c: 118770 us

RCC -O2:
  preprocess  sqlite3.c: 251157 us
  parse       sqlite3.c: 150398 us
  typecheck   sqlite3.c:  13780 us
  opt         sqlite3.c: 195639 us
  codegen     sqlite3.c: 120476 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       603 ms |
| RCC -O1   |       635 ms |
| RCC -O2   |       784 ms |
| TCC       |       118 ms |
| SLIMCC    |      1282 ms |
| KEFIR     |     23285 ms |
| KEFIR -O1 |     26056 ms |
| CCC       |     17667 ms |
| GCC -O0   |     10186 ms |
| GCC -O2   |     66063 ms |
| Clang -O0 |      2938 ms |
| Clang -O2 |     36816 ms |
