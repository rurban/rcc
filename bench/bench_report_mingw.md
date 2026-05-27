# Windows RCC Benchmark Results

_Generated: 06/06/2026 09:46:28_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3093 |          740 |       3833 |
| RCC -O1 (optimized)   |         1253 |          739 |       1992 |
| TCC (Tiny C Compiler) |         1260 |          496 |       1756 |
| GCC -O0 (no opt)      |         1257 |          471 |       1728 |
| GCC -O2 (optimized)   |         1253 |          119 |       1372 |
| CLANG -O2 (optimized) |         1257 |          188 |       1445 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.45x
- Execute speed : RCC/TCC = 1.49x
- Total : RCC/TCC = 2.18x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
