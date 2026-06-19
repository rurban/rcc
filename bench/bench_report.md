# Linux RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           51 |          608 |        659 |
| RCC -O1   |           55 |          575 |        630 |
| TCC       |            6 |          572 |        578 |
| SLIMCC    |           48 |          638 |        686 |
| KEFIR     |          192 |          680 |        872 |
| KEFIR -O1 |          187 |          753 |        940 |
| GCC -O0   |          110 |          593 |        703 |
| GCC -O2   |          178 |          212 |        390 |
| Clang -O0 |           90 |          631 |        721 |
| Clang -O2 |          133 |          233 |        366 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    415 us
  lex         bench.c:    145 us
  parse       bench.c:    331 us
  typecheck   bench.c:      7 us
  codegen     bench.c:    828 us
  peephole    bench.c:    241 us (est, 644 calls)
  link        bench_rcc:  43981 us

RCC -O1:
  preprocess  bench.c:    352 us
  lex         bench.c:    190 us
  parse       bench.c:    422 us
  typecheck   bench.c:     10 us
  opt(CTFE)   bench.c:     24 us
  codegen     bench.c:    951 us
  peephole    bench.c:    240 us (est, 640 calls)
  link        bench_rcc_o1:  44360 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 213812 us
  lex         sqlite3.c:  86157 us
  parse       sqlite3.c: 144116 us
  typecheck   sqlite3.c:  12798 us
  codegen     sqlite3.c: 393799 us
  peephole    sqlite3.c: 160332 us (est, 427553 calls)
  link        null: 1673483 us

RCC -O1:
  preprocess  sqlite3.c: 208515 us
  lex         sqlite3.c:  83623 us
  parse       sqlite3.c: 139208 us
  typecheck   sqlite3.c:  12817 us
  opt(CTFE)   sqlite3.c:  29985 us
  codegen     sqlite3.c: 384288 us
  peephole    sqlite3.c: 159251 us (est, 424671 calls)
  link        null: 1644527 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      2677 ms |
| RCC -O1   |      2670 ms |
| TCC       |       117 ms |
| SLIMCC    |      1322 ms |
| KEFIR     |     23527 ms |
| KEFIR -O1 |     26039 ms |
| GCC -O0   |     10371 ms |
| GCC -O2   |     67225 ms |
| Clang -O0 |      2965 ms |
| Clang -O2 |     37499 ms |
