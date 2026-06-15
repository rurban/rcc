# Linux RCC Benchmark Results

_Generated: Juni 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           46 |          610 |        656 |
| RCC -O1   |           40 |          604 |        644 |
| TCC       |            7 |          509 |        516 |
| SLIMCC    |           40 |          520 |        560 |
| KEFIR     |          183 |          600 |        783 |
| KEFIR -O1 |          200 |          325 |        525 |
| CCC       |           35 |          556 |        591 |
| GCC -O0   |           58 |          504 |        562 |
| GCC -O2   |          134 |          191 |        325 |
| Clang -O0 |           90 |          475 |        565 |
| Clang -O2 |          135 |          189 |        324 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    506 us
  lex         bench.c:    146 us
  parse       bench.c:    307 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   1574 us
  peephole    bench.c:    484 us
  link        bench_rcc:  36775 us

RCC -O1:
  preprocess  bench.c:    481 us
  lex         bench.c:    188 us
  parse       bench.c:    355 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     41 us
  codegen     bench.c:    861 us
  peephole    bench.c:    227 us
  link        bench_rcc_o1:  37997 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 230354 us
  lex         sqlite3.c: 114185 us
  parse       sqlite3.c: 130447 us
  typecheck   sqlite3.c:   9440 us
  codegen     sqlite3.c: 397960 us
  peephole    sqlite3.c: 155905 us
  link        null: 435191 us

RCC -O1:
  preprocess  sqlite3.c: 217150 us
  lex         sqlite3.c: 114326 us
  parse       sqlite3.c: 121252 us
  typecheck   sqlite3.c:   9611 us
  opt(CTFE)   sqlite3.c:  29186 us
  codegen     sqlite3.c: 309523 us
  peephole    sqlite3.c: 129149 us
  link        null: 431558 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1420 ms |
| RCC -O1   |      1443 ms |
| TCC       |       113 ms |
| SLIMCC    |       423 ms |
| KEFIR     |     19993 ms |
| KEFIR     |     36341 ms |
| CCC       |     13465 ms |
| GCC -O0   |      4694 ms |
| GCC -O2   |     28008 ms |
| Clang -O0 |      2041 ms |
| Clang -O2 |     22689 ms |
