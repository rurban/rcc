# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          635 |        688 |
| RCC -O1   |           49 |          613 |        662 |
| RCC -O2   |           43 |          638 |        681 |
| TCC       |            7 |          581 |        588 |
| SLIMCC    |           55 |          635 |        690 |
| KEFIR     |          225 |          680 |        905 |
| KEFIR -O1 |          205 |          500 |        705 |
| CCC       |           46 |          580 |        626 |
| GCC -O0   |           90 |          579 |        669 |
| GCC -O2   |          208 |          213 |        421 |
| Clang -O0 |          101 |          635 |        736 |
| Clang -O2 |          139 |          238 |        377 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   4323 us
  parse       bench.c:    418 us
  typecheck   bench.c:      9 us
  codegen     bench.c:    587 us
  link        bench_rcc:  35821 us

RCC -O1:
  preprocess  bench.c:   5837 us
  parse       bench.c:    607 us
  typecheck   bench.c:      9 us
  opt         bench.c:     36 us
  codegen     bench.c:    469 us
  link        bench_rcc_o1:  37128 us
```

RCC -O2:
preprocess bench.c: 4401 us
parse bench.c: 432 us
typecheck bench.c: 11 us
opt bench.c: 30 us
codegen bench.c: 363 us
link bench_rcc_o2: 41804 us

```

## RCC Substep Timing -- sqlite3.c

```

RCC:
preprocess sqlite3.c: 255827 us
parse sqlite3.c: 147659 us
typecheck sqlite3.c: 14200 us
codegen sqlite3.c: 125084 us

RCC -O1:
preprocess sqlite3.c: 268183 us
parse sqlite3.c: 159399 us
typecheck sqlite3.c: 16904 us
opt sqlite3.c: 46326 us
codegen sqlite3.c: 118977 us

```

RCC -O2:
  preprocess  sqlite3.c: 258312 us
  parse       sqlite3.c: 146970 us
  typecheck   sqlite3.c:  13921 us
  opt         sqlite3.c: 194119 us
  codegen     sqlite3.c: 119374 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       632 ms |
| RCC -O1   |       641 ms |
| RCC -O2   |       788 ms |
| TCC       |       134 ms |
| SLIMCC    |      1291 ms |
| KEFIR     |     25802 ms |
| KEFIR -O1 |     40953 ms |
| CCC       |     29506 ms |
| GCC -O0   |     16882 ms |
| GCC -O2   |     71355 ms |
| Clang -O0 |      3107 ms |
| Clang -O2 |     48458 ms |
