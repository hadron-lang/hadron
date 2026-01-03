Based on the project configuration files and source code provided, here is a strictly technical `README.md` without examples or emojis.

---

# Hadron

Hadron is a statically typed system programming language compiler targeting the LLVM backend. It supports C++23 standards and utilizes LLVM 21 for code generation.

## Dependencies

The following dependencies are required to build the compiler:

* **Operating System**: Linux
* **C++ Compiler**: Clang 21 (Must support C++23)
* **Build System**: CMake 3.25 or higher
* **Generator**: Ninja
* **LLVM**: Version 21 (Development libraries required)
* **Standard Library**: `libstdc++` (Default) or `libc++`

## Build Instructions

The project uses CMake. A `Makefile` wrapper is provided for convenience.

### Using Make

To build in Debug mode (with AddressSanitizer and UndefinedBehaviorSanitizer enabled):

```bash
make debug

```

To build in Release mode (Optimized, no sanitizers):

```bash
make release

```

To clean build artifacts:

```bash
make clean

```

### Using CMake Directly

Configure the project:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=/usr/lib/llvm-21/lib/cmake/llvm

```

Build the executable:

```bash
cmake --build build --parallel $(nproc)

```

## Usage

The compiler binary `hadronc` is located in the `build` directory.

### Command Line Interface

```bash
./build/hadronc <input_file> [options]

```

### Options

* `-o <file>`: Specify the output binary filename. Default is `a.out`.
* `--emit-ir`: Print the generated LLVM IR to stdout.
* `--dump-asm`: Display the generated assembly code using `objdump`.
* `--keep-obj`: Retain the intermediate object file (`.o`) after linking.

## Testing

Unit tests use the GoogleTest framework (fetched automatically via CMake).

To run the test suite:

```bash
make test

```

Or manually via CTest:

```bash
cd build
ctest --output-on-failure

```

## Development

### Code Formatting

The project uses `clang-format`. To format all source files:

```bash
make fmt

```

### Security

Refer to [SECURITY.md](./SECURITY.md) for vulnerability reporting policies.

## License

This software is licensed under the MIT License. See the [LICENSE](./LICENSE) file for details.