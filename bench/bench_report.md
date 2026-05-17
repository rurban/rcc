# Linux RCC Benchmark Results

_Generated: May 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           45 |          651 |        696 |
| RCC -O1   |           50 |          607 |        657 |
| TCC       |            9 |          585 |        594 |
| SLIMCC    |           48 |          636 |        684 |
| KEFIR     |          219 |          682 |        901 |
| KEFIR -O1 |          187 |          522 |        709 |
| GCC -O0   |           81 |          613 |        694 |
| GCC -O2   |          186 |          218 |        404 |
| Clang -O0 |           92 |          647 |        739 |
| Clang -O2 |          143 |          238 |        381 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    311 us
  lex         bench.c:    197 us
  parse       bench.c:    224 us
  typecheck   bench.c:      8 us
  codegen     bench.c:    636 us
  peephole    bench.c:    370 us
  link        bench_rcc:  45480 us

RCC -O1:
  preprocess  bench.c:    506 us
  lex         bench.c:    331 us
  parse       bench.c:    467 us
  typecheck   bench.c:     17 us
  opt(CTFE)   bench.c:     61 us
  codegen     bench.c:   1336 us
  peephole    bench.c:    802 us
  link        bench_rcc_o1:  39063 us
```
