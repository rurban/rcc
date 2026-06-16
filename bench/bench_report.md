# Linux RCC Benchmark Results

_Generated: Juni 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           46 |          770 |        816 |
| RCC -O1   |           48 |          685 |        733 |
| TCC       |            6 |          629 |        635 |
| SLIMCC    |           50 |          587 |        637 |
| KEFIR     |          246 |          636 |        882 |
| KEFIR -O1 |          212 |          393 |        605 |
| CCC       |           44 |          804 |        848 |
| GCC -O0   |           65 |          523 |        588 |
| GCC -O2   |          149 |          197 |        346 |
| Clang -O0 |           99 |          578 |        677 |
| Clang -O2 |          151 |          203 |        354 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    463 us
  lex         bench.c:    117 us
  parse       bench.c:    292 us
  typecheck   bench.c:      6 us
  codegen     bench.c:   1251 us
  peephole    bench.c:    465 us
  link        bench_rcc:  42140 us

RCC -O1:
  preprocess  bench.c:    446 us
  lex         bench.c:    186 us
  parse       bench.c:    323 us
  typecheck   bench.c:      6 us
  opt(CTFE)   bench.c:     27 us
  codegen     bench.c:   1143 us
  peephole    bench.c:    297 us
  link        bench_rcc_o1:  44365 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 242871 us
  lex         sqlite3.c:  88339 us
  parse       sqlite3.c: 148319 us
  typecheck   sqlite3.c:  10171 us
  codegen     sqlite3.c: 520915 us
  peephole    sqlite3.c: 224939 us
  link        null: 523251 us

RCC -O1:
  preprocess  sqlite3.c: 239580 us
  lex         sqlite3.c:  81214 us
  parse       sqlite3.c: 128726 us
  typecheck   sqlite3.c:   9649 us
  opt(CTFE)   sqlite3.c:  30656 us
  codegen     sqlite3.c: 490003 us
  peephole    sqlite3.c: 233666 us
  link        null: 551663 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1724 ms |
| RCC -O1   |      1766 ms |
| TCC       |       124 ms |
| SLIMCC    |      1147 ms |
| KEFIR     |     25455 ms |
| KEFIR     |     48609 ms |
| CCC       |     17436 ms |
| GCC -O0   |      5181 ms |
| GCC -O2   |     33508 ms |
| Clang -O0 |      2301 ms |
| Clang -O2 |     27475 ms |
