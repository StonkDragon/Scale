# [The Scale Programming Language](https://stonkdragon.github.io/)
## Introduction
Scale is a [procedual](https://en.wikipedia.org/wiki/Procedural_programming) and [object oriented](https://en.wikipedia.org/wiki/Object-oriented_programming) [concatenative](https://en.wikipedia.org/wiki/Concatenative_programming) [stack oriented](https://en.wikipedia.org/wiki/Stack-oriented_programming) [compiled](https://en.wikipedia.org/wiki/Compiler) programming language inspired by [Lua](https://www.lua.org/) and [Porth](https://gitlab.com/tsoding/porth).

Scale and [C](https://en.wikipedia.org/wiki/C_(programming_language)) can interoperate using a header file named `scale_interop.h`, which will be generated in your current working directory when compiling any Scale file.

The Compiler is a [source-to-source compiler](https://en.wikipedia.org/wiki/Source-to-source_compiler), as it converts your source code to valid C code, that is then compiled by [Clang](https://en.wikipedia.org/wiki/Clang).

Scale supports both 32-bit and 64-bit systems, but 64-bit is recommended.

### Examples

Examples can be found in the [examples](./examples) directory.

## Installation
### Install the latest release
To install the latest release, run either
```shell
$ clang++ install.cpp -o install -std=gnu++17 && ./install # works if you don't have dragon yet
```
or
```shell
$ dragon package install StonkDragon/Scale # works if you already have dragon
```

# Documentation

A list of all features can be found [here](https://stonkdragon.github.io/features.html).

The Scale Framework documentation can be viewed by running the following command:
```shell
$ sclc -doc-for Scale
```

# Build

  The Scale compiler uses [Dragon](https://github.com/StonkDragon/Dragon) as the build system. It is required to build the compiler.

  To build the compiler, execute the following command from the root directory of the repository:

```shell
$ dragon build
```

# License

  Scale is licensed under the [MIT license](./LICENSE).
