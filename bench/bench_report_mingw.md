# Windows RCC Benchmark Results

_Generated: 06/20/2026 13:49:45_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         6579 |          603 |       7182 |
| RCC -O1 (optimized)   |         1234 |          607 |       1841 |
| TCC (Tiny C Compiler) |         1248 |          429 |       1677 |
| GCC -O0 (no opt)      |         1256 |          416 |       1672 |
| GCC -O2 (optimized)   |         1245 |          115 |       1360 |
| CLANG -O2 (optimized) |         1241 |          168 |       1409 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 5.27x
- Execute speed : RCC/TCC = 1.41x
- Total : RCC/TCC = 4.28x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
