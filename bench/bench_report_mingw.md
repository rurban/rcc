# Windows RCC Benchmark Results

_Generated: 05/08/2026 09:07:20_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         9838 |          647 |      10485 |
| RCC -O1 (optimized)   |         1247 |          647 |       1894 |
| TCC (Tiny C Compiler) |         1258 |          671 |       1929 |
| GCC -O0 (no opt)      |         1258 |          570 |       1828 |
| GCC -O2 (optimized)   |         1238 |          135 |       1373 |
| CLANG -O2 (optimized) |         1269 |          314 |       1583 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 7.82x
- Execute speed : RCC/TCC = 0.96x
- Total : RCC/TCC = 5.44x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
