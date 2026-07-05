# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           60 |          621 |        681 |
| RCC -O1   |           55 |          606 |        661 |
| TCC       |            9 |          602 |        611 |
| SLIMCC    |           48 |          657 |        705 |
| KEFIR     |          219 |          691 |        910 |
| KEFIR -O1 |          225 |          514 |        739 |
| CCC       |           43 |          598 |        641 |
| GCC -O0   |           99 |          625 |        724 |
| GCC -O2   |          232 |          234 |        466 |
| Clang -O0 |          108 |          669 |        777 |
| Clang -O2 |          183 |          234 |        417 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    469 us
  lex         bench.c:    241 us
  parse       bench.c:    400 us
  typecheck   bench.c:     10 us
  codegen     bench.c:    676 us
  link        bench_rcc:  42621 us

RCC -O1:
  preprocess  bench.c:    553 us
  lex         bench.c:    240 us
  parse       bench.c:    477 us
  typecheck   bench.c:     17 us
  opt(CTFE)   bench.c:     43 us
  codegen     bench.c:    978 us
  link        bench_rcc_o1:  51919 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 291722 us
  lex         sqlite3.c: 160980 us
  parse       sqlite3.c: 200764 us
  typecheck   sqlite3.c:  18655 us
  codegen     sqlite3.c: 7236252 us

RCC -O1:
  preprocess  sqlite3.c: 244718 us
  lex         sqlite3.c: 126783 us
  parse       sqlite3.c: 160962 us
  typecheck   sqlite3.c:  13756 us
  opt(CTFE)   sqlite3.c:  37840 us
  codegen     sqlite3.c: 7610104 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      6470 ms |
| RCC -O1   |      6472 ms |
| TCC       |       159 ms |
| SLIMCC    |      1823 ms |
| KEFIR     |     28973 ms |
| KEFIR -O1 |     35362 ms |
| CCC       |     22643 ms |
| GCC -O0   |     12636 ms |
| GCC -O2   |    100282 ms |
| Clang -O0 |      4296 ms |
| Clang -O2 |     59285 ms |
