# Linux RCC Benchmark Results

_Generated: Juni 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           54 |          921 |        975 |
| RCC -O1   |           48 |          944 |        992 |
| TCC       |            7 |          763 |        770 |
| SLIMCC    |           54 |          733 |        787 |
| KEFIR     |          351 |          935 |       1286 |
| KEFIR -O1 |          314 |          503 |        817 |
| GCC -O0   |           75 |          730 |        805 |
| GCC -O2   |          179 |          233 |        412 |
| Clang -O0 |           98 |          649 |        747 |
| Clang -O2 |          187 |          248 |        435 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    564 us
  lex         bench.c:    215 us
  parse       bench.c:    290 us
  typecheck   bench.c:      9 us
  codegen     bench.c:   1342 us
  peephole    bench.c:    729 us
  link        bench_rcc:  47174 us

RCC -O1:
  preprocess  bench.c:    664 us
  lex         bench.c:    292 us
  parse       bench.c:    333 us
  typecheck   bench.c:     10 us
  opt(CTFE)   bench.c:     50 us
  codegen     bench.c:   1460 us
  peephole    bench.c:    698 us
  link        bench_rcc_o1:  47983 us
```
