# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           93 |          667 |        760 |
| RCC -O1   |           99 |          742 |        841 |
| TCC       |           98 |          737 |        835 |
| GCC -O0   |          111 |          503 |        614 |
| GCC -O2   |          147 |          318 |        465 |
| Clang -O0 |           86 |          477 |        563 |
| Clang -O2 |          124 |          280 |        404 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    625 us
  lex         bench.c:     74 us
  parse       bench.c:    175 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    199 us
  link        bench_rcc:  76070 us

RCC -O1:
  preprocess  bench.c:    625 us
  lex         bench.c:     88 us
  parse       bench.c:    129 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     16 us
  codegen     bench.c:    165 us
  link        bench_rcc_o1:  51990 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       298 ms |
| GCC -O0   |      1563 ms |
| GCC -O2   |     12983 ms |
| Clang -O0 |      1372 ms |
| Clang -O2 |     11598 ms |
