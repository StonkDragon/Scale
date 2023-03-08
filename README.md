# The Scale Programming Language

## Introduction

  Scale is a [procedual](https://en.wikipedia.org/wiki/Procedural_programming) and [object oriented](https://en.wikipedia.org/wiki/Object-oriented_programming) [concatenative](https://en.wikipedia.org/wiki/Concatenative_programming) [stack oriented](https://en.wikipedia.org/wiki/Stack-oriented_programming) [compiled](https://en.wikipedia.org/wiki/Compiler) programming language inspired by [Lua](https://www.lua.org/) and [Porth](https://gitlab.com/tsoding/porth).

  Scale and [C](https://en.wikipedia.org/wiki/C_(programming_language)) can interoperate using a header file named `scale_support.h`, which will be generated when compiling a scale file with the `-t` option. If you have any scale functions marked with the `export` modifier, C-declarations for those functions will appear in the file. Functions marked with the `expect` keyword are expected to be implemented in a different translation unit, i.e. your C-code.

  The Compiler is a [source-to-source compiler](https://en.wikipedia.org/wiki/Source-to-source_compiler), as it converts your source code to valid C code, that is then compiled by [GCC](https://en.wikipedia.org/wiki/GNU_Compiler_Collection).

  Scale supports both 32-bit and 64-bit systems, but 64-bit is recommended.

## Installation

## Install a release


### Linux & macOS
Go to the Releases tab and download the latest `source.zip`. Then:
```shell
$ unzip source.zip
$ sh install.sh
```

### Windows
Windows is not supported directly. To use Scale, follow the [Linux](#linux--macos) install instructions inside of WSL.

## Install stable dev

### Linux & macOS
```shell
$ git clone https://github.com/StonkDragon/Scale
$ sh install.sh
```

### Windows
Windows is not supported directly. To use Scale, follow the [Linux](#linux--macos) install instructions inside of WSL.

## Dependencies
  Scale has the following dependencies:

Required:
- `gcc` (or similar C compiler)
- [`dragon`](https://github.com/StonkDragon/Dragon) (The [install.sh](./install.sh) script will install this for you)

# Documentation

Run:
```console
$ sclc -doc categories
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
  - [Container Introduction](./examples/container.scale)
  - [Structure Introduction](./examples/struct.scale)
  - [C Declaration codeblock](./examples/cdecl.scale)
  - [Labels and Goto](./examples/label-goto.scale)
  - [Object-Oriented Programming in Scale](./examples/oop.scale)
  - [Foreach-Loop](./examples/foreach.scale)

# Build

  The Scale compiler uses [Dragon](https://github.com/StonkDragon/Dragon) as the build system. It is required to build the compiler.

  To build the compiler, execute the following command from the root directory of the repository:

```shell
$ dragon build
```

  This will build the compiler, move the binary to your `/usr/local/bin` folder, and run tests.

# License

  Scale is licensed under the [MIT license](./LICENSE).
