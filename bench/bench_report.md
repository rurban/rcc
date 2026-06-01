# Linux RCC Benchmark Results

_Generated: Juni 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           30 |          741 |        771 |
| RCC -O1   |           28 |          778 |        806 |
| TCC       |            5 |          586 |        591 |
| SLIMCC    |           86 |          592 |        678 |
| KEFIR     |          287 |          721 |       1008 |
| KEFIR -O1 |          242 |          384 |        626 |
| GCC -O0   |           71 |          593 |        664 |
| GCC -O2   |          191 |          212 |        403 |
| Clang -O0 |          191 |          550 |        741 |
| Clang -O2 |          263 |          213 |        476 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1214 us
  lex         bench.c:    184 us
  parse       bench.c:    210 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    274 us
  peephole    bench.c:    116 us
  link        bench_rcc:  42454 us

RCC -O1:
  preprocess  bench.c:    416 us
  lex         bench.c:    190 us
  parse       bench.c:    243 us
  typecheck   bench.c:      9 us
  opt(CTFE)   bench.c:     51 us
  codegen     bench.c:    306 us
  peephole    bench.c:    140 us
  link        bench_rcc_o1:  28181 us
```
