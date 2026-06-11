# Windows RCC Benchmark Results

_Generated: 06/12/2026 20:49:21_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3037 |          602 |       3639 |
| RCC -O1 (optimized)   |         1264 |          608 |       1872 |
| TCC (Tiny C Compiler) |         1256 |          430 |       1686 |
| GCC -O0 (no opt)      |         1251 |          419 |       1670 |
| GCC -O2 (optimized)   |         1248 |          117 |       1365 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.42x
- Execute speed : RCC/TCC = 1.4x
- Total : RCC/TCC = 2.16x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
