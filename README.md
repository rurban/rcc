# RCC: Rising C Compiler (Realtime C Compiler)

RCC is an experimental, highly optimized C compiler written from scratch in C11. 
The objective of this project is to build a compiler that eventually outperforms `TCC` (Tiny C Compiler) in both compilation speed and execution speed, employing cutting-edge techniques such as **Compile-Time Function Execution (CTFE)**.

## Key Features

- **Modern Register Machine Architecture**
  - Moves away from naive stack-machine AST compilation to dynamic register allocation (`alloc_reg` / `free_reg`), utilizing x64 generic registers efficiently.
  - Generates highly optimized assembly avoiding redundant memory tracking (`lea` elimination).
- **Compile-Time Function Execution (CTFE)** / Constexpr evaluation
  - The AST interpreter (`opt.c`) recursively evaluates pure functions internally *during compilation*.
  - Converts entire function calls with constant arguments (e.g., `fib(35)`) into mere integer literals `ND_NUM` before code generation, eliminating CPU computation overhead entirely.
- **Production-Ready Compiler Driver**
  - Standard command-line interface (`-o`, `-S`, `-c`), automatically hooking system assembler & linker (currently via GCC back-end).
  - Can safely compile source files directly incorporating standard C library `#include` by implementing a fast fallback preprocessor-bypass.
- **Windows ABI Compliance**
  - Provides rigorous alignment (16-byte boundary).
  - Perfectly handles volatile and non-volatile registers logic and `shadow space` for safe external library bindings (e.g., `printf`).

## Performance Benchmark

We executed a notoriously heavy `Fibonacci(35)` calculation benchmark across popular compilers and our own phases of development. 
Because RCC folds static deterministic code *inside* the compiler via the built-in AST interpreter, the resultant binary execution time drops to absolute zero.

| Compiler | Backend Approach | CPU Exec Time (ms) |
| --- | --- | --- |
| **GCC 15.2 (`-O0`)** | Native Runtime | ~ `263 ms` |
| **TCC 0.9.27** | Native Runtime | ~ `166 ms` |
| **RCC (Phase 7 - Register Alloc)** | Native Runtime | ~ `229 ms` |
| **RCC (Phase 8 - CTFE) 🎉** | **Compile-Time Replaced** | **`0 ms` (*33ms Proc overhead*)** |

> Note: While GCC approaches `-O2` with loop vectorizing optimization natively, our aggressive AST-Folding achieves an $O(1)$ constant execution by fully shifting the workload to compile-time.

## Build Requirements

- GCC (MinGW-w64 on Windows or standard GCC on Linux)
- Make (Optional)

## Usage

You can build the compiler simply by compiling all `src/` directory C files:
```bash
gcc -std=c11 -O2 src/*.c -o rcc.exe
```

Using the compiler acts identically to standard compilers:
```bash
# Compile and output test.exe natively
./rcc.exe test/test_real.c -o test.exe

# Output assembly to target.s
./rcc.exe test/benchmark.c -S -o target.s
```

## Structure
- `src/lexer.c` : Fast tokenizer with `#` preprocessor skipping abilities
- `src/parser.c` : Recursive descent parser generating the abstract syntax tree
- `src/type.c` : Handles primitive sets (`int`, `char`) and complex pointer arithmetic mapping
- `src/opt.c`   : A full-fledged C-language interpreter integrated internally to trigger evaluation (CTFE)
- `src/codegen.c`: Tree-based x64 Register Allocator compliant with Windows Standard conventions

---
*Developed as an initiative to transcend TCC limits.*
