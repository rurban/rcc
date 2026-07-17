# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           49 |          569 |        618 |
| RCC -O1   |           49 |          623 |        672 |
| TCC       |           41 |          511 |        552 |
| GCC -O0   |           59 |          433 |        492 |
| GCC -O2   |           92 |          263 |        355 |
| Clang -O0 |           53 |          432 |        485 |
| Clang -O2 |           91 |          262 |        353 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    684 us
  lex         bench.c:     54 us
  parse       bench.c:    112 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    118 us
  link        bench_rcc:  46818 us

RCC -O1:
  preprocess  bench.c:    395 us
  lex         bench.c:     86 us
  parse       bench.c:    120 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     18 us
  codegen     bench.c:    144 us
  link        bench_rcc_o1:  44767 us
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
| TCC       |        83 ms |
| GCC -O0   |       888 ms |
| GCC -O2   |      8692 ms |
| Clang -O0 |       949 ms |
| Clang -O2 |      8740 ms |
