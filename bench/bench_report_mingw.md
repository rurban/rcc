# Windows RCC Benchmark Results

_Generated: 06/10/2026 13:45:07_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3076 |          737 |       3813 |
| RCC -O1 (optimized)   |         1252 |          735 |       1987 |
| TCC (Tiny C Compiler) |         1250 |          497 |       1747 |
| GCC -O0 (no opt)      |         1254 |          467 |       1721 |
| GCC -O2 (optimized)   |         1254 |          118 |       1372 |
| CLANG -O2 (optimized) |         1289 |          188 |       1477 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.46x
- Execute speed : RCC/TCC = 1.48x
- Total : RCC/TCC = 2.18x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
