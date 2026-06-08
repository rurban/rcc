# Windows RCC Benchmark Results

_Generated: 06/08/2026 13:11:38_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2930 |          605 |       3535 |
| RCC -O1 (optimized)   |         1247 |          600 |       1847 |
| TCC (Tiny C Compiler) |         1287 |          429 |       1716 |
| GCC -O0 (no opt)      |         1248 |          416 |       1664 |
| GCC -O2 (optimized)   |         1245 |          116 |       1361 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.28x
- Execute speed : RCC/TCC = 1.41x
- Total : RCC/TCC = 2.06x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
