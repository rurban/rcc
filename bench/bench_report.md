# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           41 |          628 |        669 |
| RCC -O1   |           40 |          614 |        654 |
| TCC       |            7 |          573 |        580 |
| SLIMCC    |           51 |          630 |        681 |
| KEFIR     |          237 |          674 |        911 |
| KEFIR -O1 |          208 |          504 |        712 |
| CCC       |           63 |          622 |        685 |
| GCC -O0   |           72 |          585 |        657 |
| GCC -O2   |          215 |          233 |        448 |
| Clang -O0 |          130 |          677 |        807 |
| Clang -O2 |          150 |          233 |        383 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2765 us
  lex         bench.c:    281 us
  parse       bench.c:    628 us
  typecheck   bench.c:     15 us
  codegen     bench.c:    913 us
  link        bench_rcc:  34483 us

RCC -O1:
  preprocess  bench.c:   4609 us
  lex         bench.c:    412 us
  parse       bench.c:    411 us
  typecheck   bench.c:      8 us
  opt(CTFE)   bench.c:     17 us
  codegen     bench.c:    521 us
  link        bench_rcc_o1:  29332 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 172068 us
  lex         sqlite3.c: 102173 us
  parse       sqlite3.c: 155438 us
  typecheck   sqlite3.c:  14867 us
  codegen     sqlite3.c: 208197 us

RCC -O1:
  preprocess  sqlite3.c: 189335 us
  lex         sqlite3.c: 109695 us
  parse       sqlite3.c: 150789 us
  typecheck   sqlite3.c:  13921 us
  opt(CTFE)   sqlite3.c:  33300 us
  codegen     sqlite3.c: 199790 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       714 ms |
| RCC -O1   |       833 ms |
| TCC       |       122 ms |
| SLIMCC    |      1348 ms |
| KEFIR     |     25204 ms |
| KEFIR -O1 |     28691 ms |
| CCC       |     23305 ms |
| GCC -O0   |     15268 ms |
| GCC -O2   |     82155 ms |
| Clang -O0 |      3106 ms |
| Clang -O2 |     40277 ms |
