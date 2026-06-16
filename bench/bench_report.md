# Linux RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           47 |          569 |        616 |
| RCC -O1   |           51 |          568 |        619 |
| TCC       |            6 |          570 |        576 |
| SLIMCC    |           39 |          625 |        664 |
| KEFIR     |          179 |          658 |        837 |
| KEFIR -O1 |          190 |          503 |        693 |
| CCC       |           34 |          565 |        599 |
| GCC -O0   |           69 |          570 |        639 |
| GCC -O2   |          174 |          216 |        390 |
| Clang -O0 |           88 |          627 |        715 |
| Clang -O2 |          129 |          235 |        364 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    418 us
  lex         bench.c:    148 us
  parse       bench.c:    331 us
  typecheck   bench.c:      7 us
  codegen     bench.c:   1167 us
  peephole    bench.c:    241 us (est, 644 calls)
  link        bench_rcc:  38796 us

RCC -O1:
  preprocess  bench.c:    492 us
  lex         bench.c:    207 us
  parse       bench.c:    529 us
  typecheck   bench.c:     14 us
  opt(CTFE)   bench.c:     32 us
  codegen     bench.c:    918 us
  peephole    bench.c:    240 us (est, 640 calls)
  link        bench_rcc_o1:  38334 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 201786 us
  lex         sqlite3.c:  84370 us
  parse       sqlite3.c: 143449 us
  typecheck   sqlite3.c:  12983 us
  codegen     sqlite3.c: 345740 us
  peephole    sqlite3.c: 160092 us (est, 426912 calls)
  link        null: 1596800 us

RCC -O1:
  preprocess  sqlite3.c: 198545 us
  lex         sqlite3.c:  83161 us
  parse       sqlite3.c: 140265 us
  typecheck   sqlite3.c:  12927 us
  opt(CTFE)   sqlite3.c:  29665 us
  codegen     sqlite3.c: 350919 us
  peephole    sqlite3.c: 159011 us (est, 424030 calls)
  link        null: 1597991 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      2669 ms |
| RCC -O1   |      2678 ms |
| TCC       |       109 ms |
| SLIMCC    |      1263 ms |
| KEFIR     |     23443 ms |
| KEFIR -O1 |     25606 ms |
| CCC       |     18330 ms |
| GCC -O0   |     10155 ms |
| GCC -O2   |     65283 ms |
| Clang -O0 |      2896 ms |
| Clang -O2 |     36554 ms |
