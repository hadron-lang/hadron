<div align="center">
  <h1>Hadron</h1>
</div>

<div id="badges" align="center">
  <a href="https://github.com/hadron-lang/hadron/stargazers"><img alt="Stars" src="https://img.shields.io/github/stars/hadron-lang/hadron?colorA=333&colorB=ffa&style=for-the-badge"></a>
  <a href="https://github.com/hadron-lang/hadron/issues"><img alt="Issues" src="https://img.shields.io/github/issues/hadron-lang/hadron?colorA=333&colorB=faa&style=for-the-badge"></a>
  <a href="https://github.com/hadron-lang/hadron/contributors"><img alt="Contributors" src="https://img.shields.io/github/contributors/hadron-lang/hadron?colorA=333&colorB=aaf&style=for-the-badge"></a>
</div>

<h3><i>

Hello and welcome to Hadron :wave:, an early-stage language development project! We're thrilled that you've stopped
by our project and hope you'll consider trying it out.

We welcome any ideas, feedback, and contributions from the community. Whether you're a seasoned developer or just
starting out, we'd love to hear from you.

Please take a look at our documentation and feel free to reach out to us if you have any questions.

</i></h3>

---

Hadron is a custom programming language written in C++. It is designed to be a versatile language that can be
transpiled (see [notes](#development-notes)) or interpreted. It aims to provide a simple and intuitive syntax, while
still being powerful and flexible.

## Planned Features

- Simple and intuitive syntax
- Static type system
- On-demand garbage collection
- Support for closures and anonymous functions
- First-class functions
- Modules and namespaces
- Interoperability with C code
- Transpilation to C, JavaScript, Python, and other languages

## Getting Started

To build Hadron, you need to have `clang` installed on your system. You can build Hadron by running the following
commands:

```sh
git clone https://github.com/hadron-lang/hadron.git
cd hadron
cmake -B build
cmake --build build
```

To run the interpreter, you can use the following commands:

```sh
./build/hadron
```

To compile to bytecode, you can use the following command:

```sh
./build/hadron input.hdn # compiled to `input.hbc`
```

Hadron will attempt to execute any `.hbc` file. For example:

```sh
./build/hadron input.hbc
```

## Examples

_Please note that the syntax may change in the future._

Here is an example of a "Hello, world!" program in Hadron:

```
fx main() {
  IO.out("Hello, world!")
}
```

And here is an example of a function that calculates the nth Fibonacci number:

```
fx fib(i32 n) {
  if n < 2 {
    return n
  }
  return fib(n-2) + fib(n-1);
} i32

fx main() {
  IO.out(fib(10));
}
```

## Roadmap

| **Feature**                    | **Status**     | **Details**                                                                                                                     |
|--------------------------------|----------------|---------------------------------------------------------------------------------------------------------------------------------|
| Basic Mathematical Expressions | ✅ Complete     | Supports `+`, `-`, `*`, `/`, and parentheses for grouping.                                                                      |
| Numbers                        | ✅ Complete     | Support for different number syntaxes such as `0xFF`, `0b1010`, `0x.8p1` and others.                                            |
| Logical and Binary Expressions | ⚠️ In Progress | Support for logical operators `!`, `&&`, <code>&#124;&#124;</code> and binary operators `~`, `&`, <code>&#124;</code>, and `^`. |
| Variable Declarations          | ⚠️ In Progress | Syntax: `i32 a = 1 + 2;`.                                                                                                       |
| Control Flow                   | ❌ Not Started  | `if`, `for`, `while`, and `switch` expressions.                                                                                 |
| Function Definitions           | ❌ Not Started  | Syntax for `fx name(args) {}` and return types.                                                                                 |
| Number Ranges                  | ❌ Not Started  | Support for range operators `..`, `=..`, `..=`, and `=..=`.                                                                     |
| Standard Library Integration   | ❌ Not Started  | Namespace `IO`, strings, arrays, and utilities.                                                                                 |
| Type Inference                 | ❌ Not Started  | Implicit types with `x $= 42`.                                                                                                  |
| Asynchronous Execution         | ❌ Not Started  | Create and execute asynchronous functions using `async` and `await`                                                             |

## Development Notes

In its current state, transpilation has not yet been implemented. This is due to the priority given to building and
refining the interpreter, which serves as the foundation for executing the language's bytecode. Key focus areas include:

- **Optimizing the Interpreter**: Ensuring efficient execution of bytecode, with minimal runtime overhead.
- **Defining Core Features**: Establishing a robust syntax and semantics for the language, including parsing and symbol
  resolution.
- **Bytecode Stability**: Finalizing the bytecode format to maintain compatibility as the project evolves.

Future plans include adding a transpilation layer, which will allow Hadron code to be converted to other languages or
directly to native assembly. This will be addressed after the interpreter achieves sufficient maturity and performance
benchmarks are met. For now, the focus remains on creating a stable and feature-complete execution environment.

## Contributing

We welcome contributions to Hadron! If you find a bug or have an idea for a new feature, please contact
us [here](https://hadronlang.com/feedback) or open an issue on our GitHub repository. If you would like to contribute
code, please submit a pull request. Before submitting a pull request, please make sure your code follows our coding
style and passes our tests.

## License

Hadron is released under the MIT License. See [LICENSE](LICENSE) for details.
