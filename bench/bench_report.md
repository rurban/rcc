# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           52 |          639 |        691 |
| RCC -O1   |           51 |          619 |        670 |
| TCC       |           15 |          578 |        593 |
| SLIMCC    |           52 |          645 |        697 |
| KEFIR     |          224 |          678 |        902 |
| KEFIR -O1 |          229 |          497 |        726 |
| CCC       |           62 |          563 |        625 |
| GCC -O0   |           83 |          566 |        649 |
| GCC -O2   |          190 |          215 |        405 |
| Clang -O0 |           99 |          622 |        721 |
| Clang -O2 |          151 |          234 |        385 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   4030 us
  parse       bench.c:    453 us
  typecheck   bench.c:     11 us
  codegen     bench.c:    508 us
  link        bench:    41633 us

RCC -O1:
  preprocess  bench.c:   4215 us
  parse       bench.c:    473 us
  typecheck   bench.c:     11 us
  opt(CTFE)   bench.c:     21 us
  codegen     bench.c:    414 us
  link        bench_o1: 38308 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 259227 us
  parse       sqlite3.c: 144888 us
  typecheck   sqlite3.c:  14028 us
  codegen     sqlite3.c: 119232 us

RCC -O1:
  preprocess  sqlite3.c: 269230 us
  parse       sqlite3.c: 142854 us
  typecheck   sqlite3.c:  14139 us
  opt(CTFE)   sqlite3.c:  32073 us
  codegen     sqlite3.c: 125969 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       597 ms |
| RCC -O1   |       626 ms |
| TCC       |       141 ms |
| SLIMCC    |      1326 ms |
| KEFIR     |     23844 ms |
| KEFIR -O1 |     26488 ms |
| CCC       |     18885 ms |
| GCC -O0   |     10645 ms |
| GCC -O2   |     68493 ms |
| Clang -O0 |      3073 ms |
| Clang -O2 |     38009 ms |
