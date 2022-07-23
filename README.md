# The Scale Programming Language

## Introduction

  Scale is a [functional](https://en.wikipedia.org/wiki/Functional_programming) [concatenative](https://en.wikipedia.org/wiki/Concatenative_programming) [stack-based](https://en.wikipedia.org/wiki/Stack-oriented_programming) [compiled](https://en.wikipedia.org/wiki/Compiler) programming language inspired by [Porth](https://gitlab.com/tsoding/porth).

  Scale and [C](https://en.wikipedia.org/wiki/C_(programming_language)) can interoperate using the [scale.h](./Scale/comp/scale.h) header file, which supplies function declarations for all of the Scale functions. It also contains Macros for calling your own Scale functions from C.

  The Compiler is a [source-to-source compiler](https://en.wikipedia.org/wiki/Source-to-source_compiler), as it converts your source code to valid C code, that is then compiled by [GCC](https://en.wikipedia.org/wiki/GNU_Compiler_Collection).

## Installation

  To install Scale, run the following commands:

```bash
git clone https://github.com/StonkDragon/Scale.git
cd Scale
make install
```

  The `make install` command will build the compiler and the Scale library.

  After building you have to add the following path to your PATH environment variable in order to use the compiler:

  [On Windows](https://docs.microsoft.com/en-us/previous-versions/office/developer/sharepoint-2010/ee537574(v=office.14)#to-add-a-path-to-the-path-environment-variable): `C:/Users/<your username>/Scale/bin`

  [On Mac](https://www.architectryan.com/2012/10/02/add-to-the-path-on-mac-os-x-mountain-lion/): `/Users/<your username>/Scale/bin`

  [On Linux](https://www.cyberciti.biz/faq/how-to-add-to-bash-path-permanently-on-linux/): `/home/<your username>/Scale/bin`

## Dependencies

  The following dependencies are required to use Scale:

- git
- make
- clang

## Argument Notation
```
arg3 arg2 arg1 function
```

# Documentation

- [stdlib.scale](./docs/stdlib.scale.md)

- [math.scale](./docs/math.scale.md)

- [file.scale](./docs/file.scale.md)

- [debug.scale](./docs/debug.scale.md)
