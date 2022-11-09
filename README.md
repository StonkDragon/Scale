# The Scale Programming Language

## Introduction

  Scale is a [procedual](https://en.wikipedia.org/wiki/Procedural_programming) [concatenative](https://en.wikipedia.org/wiki/Concatenative_programming) [stack-based](https://en.wikipedia.org/wiki/Stack-oriented_programming) [compiled](https://en.wikipedia.org/wiki/Compiler) programming language inspired by [Porth](https://gitlab.com/tsoding/porth).

  Scale and [C](https://en.wikipedia.org/wiki/C_(programming_language)) can interoperate using a header file named `scale_support.h`, which will be generated when compiling a scale file. This file contains C-declarations for all important Scale internal functions and data types. If you have any scale functions marked with the `nomangle` modifier, C-declarations for those functions will also appear in the file. The `scl_export`-Macro defined in said file allows you to declare functions that can be called from scale.

  The Compiler is a [source-to-source compiler](https://en.wikipedia.org/wiki/Source-to-source_compiler), as it converts your source code to valid C code, that is then compiled by [GCC](https://en.wikipedia.org/wiki/GNU_Compiler_Collection).

## Installation

  To install Scale, download the zip file for your operating system from the Release page.

  Then, unzip the file and execute the install.py script. On Windows, you have to run it with Administrator permissions. On macOS and Linux, the script will ask for permissions once it needs them.

  Reason: `runas` requires name of an Administrator account and the Hostname on Windows

## Dependencies

  Scale has the following dependencies:

Required:
- `gcc`
- `python3` ([install.py](./install.py))

Optional:
- [`dragon`](https://github.com/StonkDragon/Dragon) (for compiling the Compiler)

# Documentation

## Operators

  The following operators are supported:

  - `+`: Addition (Prefix with `.` to operate on floats)
  - `-`: Subtraction (Prefix with `.` to operate on floats)
  - `*`: Multiplication (Prefix with `.` to operate on floats)
  - `/`: Division (Prefix with `.` to operate on floats)
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
  - `->`: Container Access
  - `::`: Structure Dereference

## Keywords

  - `function`: Define a function
  - `end`: End a function
  - `extern`: Declare an externally defined function or global variable
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
  - `store`: Store the top of the stack in the following variable
  - `decl`: Declare a variable
  - `addr`: Get the address of the following identifier
  - `deref`: Push the value at the address on the stack
  - `ref`: Write the value at the address on the stack to the following variable
  - `container`: Declare a new Container to hold multiple values
  - `repeat`: Define repeat iterator loop
  - `struct`: Define a struct
  - `new`: Create a new value of a Struct type
  - `is`: Check if a value is of a specific type
  - `cdecl`: Interprets the following string literal as C-code
  - `label`: Declare a new label
  - `goto`: Goto a label

## Argument Notation/Calling Convention

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

  - [Repeat](./examples/repeat.scale)

  - [FizzBuzz](./examples/fizzbuzz.scale)

  - [Fibonacci](./examples/fib.scale)

  - [Sizeof](./examples/sizeof.scale)

  - [Stack Autodrop Pool](./examples/sap.scale)

  - [Containers](./examples/container.scale)

  - [Struct](./examples/struct.scale)

  - [Pointer Dereferencing](./examples/deref.scale)

# Build

  The Scale compiler uses [Dragon](https://github.com/StonkDragon/Dragon) as the build system. It is required to build the compiler.

  To build the compiler, execute the following command from the root directory of the repository:

```bash
$ dragon build
```

  This will build the compiler, move the binary to your `/usr/local/bin` folder, and run tests.

# License

  Scale is licensed under the [MIT license](./LICENSE).
