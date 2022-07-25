# The Scale Programming Language

## Introduction

  Scale is a [functional](https://en.wikipedia.org/wiki/Functional_programming) [concatenative](https://en.wikipedia.org/wiki/Concatenative_programming) [stack-based](https://en.wikipedia.org/wiki/Stack-oriented_programming) [compiled](https://en.wikipedia.org/wiki/Compiler) programming language inspired by [Porth](https://gitlab.com/tsoding/porth).

  Scale and [C](https://en.wikipedia.org/wiki/C_(programming_language)) can interoperate using the [scale.h](./Scale/comp/scale.h) header file, which supplies function declarations for all of the Scale functions. It also contains Macros for calling your own Scale functions from C.

  The Compiler is a [source-to-source compiler](https://en.wikipedia.org/wiki/Source-to-source_compiler), as it converts your source code to valid C code, that is then compiled by [Clang](https://en.wikipedia.org/wiki/Clang).

## Installation

  To install Scale, run the following commands:

```bash
git clone https://github.com/StonkDragon/Scale.git
cd Scale
make install
```

  The `make install` command will build the compiler and the Scale library.

  The Scale compiler executable is located in `$HOME/Scale/bin/sclc`.

  After building you have to add the following path to your PATH environment variable in order to use the compiler:

  [On Windows](https://docs.microsoft.com/en-us/previous-versions/office/developer/sharepoint-2010/ee537574(v=office.14)#to-add-a-path-to-the-path-environment-variable): `C:/Users/<your username>/Scale/bin`

  [On Mac](https://www.architectryan.com/2012/10/02/add-to-the-path-on-mac-os-x-mountain-lion/): `/Users/<your username>/Scale/bin`

  [On Linux](https://www.cyberciti.biz/faq/how-to-add-to-bash-path-permanently-on-linux/): `/home/<your username>/Scale/bin`

## Dependencies

  The following dependencies are required to use Scale:

- git
- make
- clang

# Documentation

## Operators

  The following operators are supported:

  - `:`: Define Variable
  - `>`: Store Variable
  - `*`: Load Variable
  - `&`: Load Pointer

### :

  The `:` operator is used to define a variable.

Usage:

```
:var
```

Result: The variable `var` is defined and can be used in subsequent expressions.

### >

  The `>` operator stores the value on top of the stack into the variable with the identifier on the right.

Usage:

```
"Hello, World!" >var
```
Result: The Variable `var` now holds a pointer to the string `"Hello, World!"`.

### *

  The `*` operator loads the value from the variable with the identifier on the right into the stack.

  Usage:

  ```
  *var
  ```

  Result: The value of the Variable `var` is pushed onto the stack. (In this case, a pointer to the string `"Hello, World!"`.)

### &

  The `&` operator pushes the address of the identifier on the right onto the stack. This is useful for getting function pointers.

  Usage:

  ```
  &func
  ```

  Result: The address of the function `func` is pushed onto the stack.

## Argument Notation
```
arg3 arg2 arg1 function
```

## Standard Library

- [stdlib.scale](./docs/stdlib.scale.md)

- [math.scale](./docs/math.scale.md)

- [file.scale](./docs/file.scale.md)

- [debug.scale](./docs/debug.scale.md)
