# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           45 |          604 |        649 |
| RCC -O1   |           47 |          624 |        671 |
| RCC -O2   |           50 |          637 |        687 |
| TCC       |            6 |          579 |        585 |
| SLIMCC    |           47 |          649 |        696 |
| KEFIR     |          206 |          670 |        876 |
| KEFIR -O1 |          209 |          497 |        706 |
| CCC       |           53 |          574 |        627 |
| GCC -O0   |           92 |          571 |        663 |
| GCC -O2   |          179 |          213 |        392 |
| Clang -O0 |           98 |          642 |        740 |
| Clang -O2 |          147 |          236 |        383 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   5216 us
  parse       bench.c:    672 us
  typecheck   bench.c:     16 us
  codegen     bench.c:    790 us
  link        bench_rcc:  44778 us

RCC -O1:
  preprocess  bench.c:   5633 us
  parse       bench.c:    542 us
  typecheck   bench.c:     11 us
  opt         bench.c:     36 us
  codegen     bench.c:    536 us
  link        bench_rcc_o1:  42844 us
```

RCC -O2:
preprocess bench.c: 7091 us
parse bench.c: 602 us
typecheck bench.c: 9 us
opt bench.c: 35 us
codegen bench.c: 444 us
link bench_rcc_o2: 38440 us

```

## RCC Substep Timing -- sqlite3.c

```

RCC:
preprocess sqlite3.c: 259207 us
parse sqlite3.c: 153340 us
typecheck sqlite3.c: 14203 us
codegen sqlite3.c: 116032 us

RCC -O1:
preprocess sqlite3.c: 274501 us
parse sqlite3.c: 150435 us
typecheck sqlite3.c: 13767 us
opt sqlite3.c: 43050 us
codegen sqlite3.c: 122236 us

```

RCC -O2:
  preprocess  sqlite3.c: 250781 us
  parse       sqlite3.c: 147379 us
  typecheck   sqlite3.c:  13795 us
  opt         sqlite3.c: 191926 us
  codegen     sqlite3.c: 126635 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       619 ms |
| RCC -O1   |       639 ms |
| RCC -O2   |       803 ms |
| TCC       |       115 ms |
| SLIMCC    |      1280 ms |
| KEFIR     |     23319 ms |
| KEFIR -O1 |     25821 ms |
| CCC       |     17579 ms |
| GCC -O0   |     10233 ms |
| GCC -O2   |     67260 ms |
| Clang -O0 |      2950 ms |
| Clang -O2 |     36699 ms |
