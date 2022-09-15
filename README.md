# The Scale Programming Language

## Introduction

  Scale is a [procedual](https://en.wikipedia.org/wiki/Procedural_programming) [concatenative](https://en.wikipedia.org/wiki/Concatenative_programming) [stack-based](https://en.wikipedia.org/wiki/Stack-oriented_programming) [compiled](https://en.wikipedia.org/wiki/Compiler) programming language inspired by [Porth](https://gitlab.com/tsoding/porth).

  Scale and [C](https://en.wikipedia.org/wiki/C_(programming_language)) can interoperate using the [scale.h](./Scale/Frameworks/Core.framework/impl/scale.h) header file, which supplies function declarations for all of the Scale functions. It also contains Macros for calling your own Scale functions from C.

  The Compiler is a [source-to-source compiler](https://en.wikipedia.org/wiki/Source-to-source_compiler), as it converts your source code to valid C code, that is then compiled by [Clang](https://en.wikipedia.org/wiki/Clang).

## Installation

  To install Scale, download the zip file for your operating system from the Release page.

  Then, unzip the file and execute the install.py script. On Windows, you have to run it with Administrator permissions. On macOS and Linux, the script will ask for permissions once it needs them.

  Reason: `runas` requires name of an Administrator account and the Hostname on Windows

## Dependencies

  Scale has the following dependencies:

Required:
- [`clang`](https://clang.llvm.org/)
- `cpp`
- `python3` ([install.py](./install.py))

Optional:
- [`dragon`](https://github.com/StonkDragon/Dragon) (for compiling the Compiler)

# Documentation

## Operators

  The following operators are supported:

  - `+`: Addition
  - `-`: Subtraction
  - `*`: Multiplication
  - `/`: Division
  - `%`: Modulo
  - `&`: Logical AND
  - `|`: Logical OR
  - `^`: Logical XOR
  - `~`: Logical NOT
  - `<<`: Left shift
  - `>>`: Right shift
  - `**`: Exponentiation

  - `[`: Stack Autodrop Pool Open
  - `]`: Stack Autodrop Pool Close

## Keywords

  - `function`: Define a function
  - `end`: End a function
  - `extern`: Declare a C function prototype
  - `while`: Begin a while loop header
  - `else`: Define an else block
  - `do`: End a while loop header and begin the loop body
  - `done`: End a loop body
  - `if`: Begin an if statement
  - `fi`: End an if statement
  - `return`: Return from a function
  - `break`: Break out of a loop
  - `continue`: Continue to the next iteration of a loop
  - `for`: Begin a for loop header
  - `in`: Define for loop iterator
  - `to`: Define for loop end value
  - `proto`: Define a function prototype (Scale Funcion)
  - `store`: Store the top of the stack in the following variable
  - `decl`: Declare a variable
  - `addr`: Get the address of the following identifier
  - `deref`: Push the value at the address on the stack
  - `ref`: Write the value at the address on the stack to the following variable
  - `container`: Declare a new Container to hold multiple values

## Argument Notation

Say we have following function:

```
function foo(a, b, c)
  a b +
  c +
end
```

To Call the function, the arguments must be passed in the following order:

```
  a b c foo
```

The last argument of the function must be on the top of the stack.

Example:

```
  1 2 3 foo
    => 6
```

## Stack Notation

  The following notation is used to represent the stack:

  - `[]`: An empty stack

  - `[a]`: A stack with the value `a` on top

  - `[a b]`: A stack with two elements, where `b` is on top and `a` is below

  - `[<any>]`: A stack with any number of elements

## Examples

  Examples can be found in the [examples](./examples) directory.

  Quick links:

  - [Hello World](./examples/hello.scale)

  - [Argument Notation](./examples/arguments.scale)

  - [If Statements](./examples/if.scale)

  - [While Loops](./examples/while.scale)

  - [Math Operators](./examples/operators.scale)

  - [Variables](./examples/variables.scale)

  - [For Loops](./examples/for.scale)

  - [FizzBuzz](./examples/fizzbuzz.scale)

  - [Fibonacci](./examples/fib.scale)

  - [Sizeof](./examples/sizeof.scale)

  - [Stack Autodrop Pool](./examples/sap.scale)

  - [Containers](./examples/container.scale)

  - [Pointer Dereferencing](./examples/deref.scale)

# Build

  The Scale compiler uses [Dragon](https://github.com/StonkDragon/Dragon) as the build system. It is required to build the compiler.

  To build the compiler, execute the following command from the root directory of the repository:

```bash
$ dragon build
```

  This will build the compiler and put the binary in the `compile` directory.

# License

  Scale is licensed under the [MIT license](./LICENSE).
