# The Scale Programming Language

## Introduction

  Scale is a [procedual](https://en.wikipedia.org/wiki/Procedural_programming) [concatenative](https://en.wikipedia.org/wiki/Concatenative_programming) [stack oriented](https://en.wikipedia.org/wiki/Stack-oriented_programming) [compiled](https://en.wikipedia.org/wiki/Compiler) programming language inspired by [Porth](https://gitlab.com/tsoding/porth).

  Scale and [C](https://en.wikipedia.org/wiki/C_(programming_language)) can interoperate using a header file named `scale_support.h`, which will be generated when compiling a scale file with the `-t` option. This file contains C-declarations for all important Scale internal functions and data types. If you have any scale functions marked with the `nomangle` modifier, C-declarations for those functions will also appear in the file. The `scl_export`-Macro defined in said file allows you to declare functions that can be called from scale.

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

Run:
```console
$ sclc -doc info
```

## Scale Framework Documentation

Run:
```console
$ sclc -doc-for Scale
```

## Examples

  Examples can be found in the [examples](./examples) directory.

  Here is a list of examples that should explain some of the syntax and practices in Scale:

  - [Hello World](./examples/hello.scale)
  - [Operators](./examples/operators.scale)
  - [If-Statement](./examples/if.scale)
  - [Variables in Scale](./examples/variables.scale)
  - [While-Loop](./examples/while.scale)
  - [Fibonacci Numbers](./examples/fib.scale)
  - [For-Loop](./examples/for.scale)
  - [Switch Expression](./examples/switch.scale)
  - [For-Loop with step](./examples/for-step.scale)
  - [Repeat-Block](./examples/repeat.scale)
  - [FizzBuzz](./examples/fizzbuzz.scale)
  - [Function Arguments](./examples/arguments.scale)
  - [Pointer Dereferencing](./examples/deref.scale)
  - [Container Introduction](./examples/container.scale)
  - [Structure Introduction](./examples/struct.scale)
  - [C Declaration codeblock](./examples/cdecl.scale)
  - [Labels and Goto](./examples/label-goto.scale)
  - [Object-Oriented Programming in Scale](./examples/oop.scale)
  - [Foreach-Loop](./examples/foreach.scale)

# Build

  The Scale compiler uses [Dragon](https://github.com/StonkDragon/Dragon) as the build system. It is required to build the compiler.

  To build the compiler, execute the following command from the root directory of the repository:

```bash
$ dragon build
```

  This will build the compiler, move the binary to your `/usr/local/bin` folder, and run tests.

# License

  Scale is licensed under the [MIT license](./LICENSE).
