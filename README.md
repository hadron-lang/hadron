:warning: **This project is still in progress, most of the features described here are not implemented** :warning:

# hadron

hadron is a custom programming language written in C. It is designed to be a versatile language that can be transcompiled to other languages or interpreted. It aims to provide a simple and intuitive syntax, while still being powerful and flexible.

## Planned Features
 - Simple and intuitive syntax
 - Dynamic type system
 - Garbage collection
 - Support for closures and anonymous functions
 - First-class functions
 - Modules and namespaces
 - Interoperability with C code
 - Transcompilation to C, JavaScript, Python, and other languages

## Getting Started

To build hadron, you need to have a C compiler installed on your system. You can build hadron by running the following commands:

```sh
git clone https://github.com/webd3vs/hadron.git
cd hadron
make
```

To run the interpreter, you can use the following command:

```sh
./hadron
```

To transcompile to C, you can use the following command:

```sh
./hadron -l c input.hadron -o output.c
```

To transcompile to other languages, you can replace c with the target language. For example:
```sh
./hadron -l javascript input.hadron -o output.js
```
## Examples

Here is an example of a "Hello, world!" program in hadron:

```py
func main() {
	print("Hello, world!")
}
```

And here is an example of a function that calculates the nth Fibonacci number:

```py
func fib(n) {
  if (n < 2) return n;
  else return fib(n-1) + fib(n-2);
}

func main() {
	print(fib(10));
}
```
**_Please note that the syntax may change in the future._**

## Contributing

We welcome contributions to hadron! If you find a bug or have an idea for a new feature, please open an issue on our GitHub repository. If you would like to contribute code, please submit a pull request. Before submitting a pull request, please make sure your code follows our coding style and passes our tests.

## License

hadron is released under the MIT License. See LICENSE for details.
