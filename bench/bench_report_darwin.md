# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          101 |          646 |        747 |
| RCC -O1   |          132 |          605 |        737 |
| TCC       |           63 |          596 |        659 |
| GCC -O0   |          142 |          500 |        642 |
| GCC -O2   |          140 |          281 |        421 |
| Clang -O0 |          147 |          479 |        626 |
| Clang -O2 |          137 |          312 |        449 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    811 us
  lex         bench.c:    102 us
  parse       bench.c:     91 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   1437 us
  peephole    bench.c:    236 us
  link        bench_rcc:  67638 us

RCC -O1:
  preprocess  bench.c:    775 us
  lex         bench.c:    102 us
  parse       bench.c:    104 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:   1918 us
  peephole    bench.c:    239 us
  link        bench_rcc_o1:  66846 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 416222 us
  lex         sqlite3.c: 171632 us
  parse       sqlite3.c: 149374 us
  typecheck   sqlite3.c:  21176 us
  codegen     sqlite3.c: 1334641 us
  peephole    sqlite3.c: 220848 us
  link        null: 6830854 us

RCC -O1:
  preprocess  sqlite3.c: 715659 us
  lex         sqlite3.c: 163764 us
  parse       sqlite3.c: 123345 us
  typecheck   sqlite3.c:  24159 us
  opt(CTFE)   sqlite3.c:  24901 us
  codegen     sqlite3.c: 597993 us
  peephole    sqlite3.c: 169598 us
  link        null: 5598817 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      7474 ms |
| RCC -O1   |      7320 ms |
| TCC       |       149 ms |
| GCC -O0   |      1540 ms |
| GCC -O2   |     17901 ms |
| Clang -O0 |      2030 ms |
| Clang -O2 |     15809 ms |
