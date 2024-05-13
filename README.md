# The Scale Programming Language
## Introduction
Scale is a [procedural](https://en.wikipedia.org/wiki/Procedural_programming) and [object oriented](https://en.wikipedia.org/wiki/Object-oriented_programming) [concatenative](https://en.wikipedia.org/wiki/Concatenative_programming) [stack oriented](https://en.wikipedia.org/wiki/Stack-oriented_programming) [compiled](https://en.wikipedia.org/wiki/Compiler) programming language inspired by [Lua](https://www.lua.org/) and [Porth](https://gitlab.com/tsoding/porth).

The Compiler is a [source-to-source compiler](https://en.wikipedia.org/wiki/Source-to-source_compiler), as it converts your source code to valid C code, that is then compiled by [Clang](https://en.wikipedia.org/wiki/Clang).

Scale supports both 32-bit and 64-bit systems, but 64-bit is strongly recommended.

### Examples

Examples can be found in the [examples](./examples) directory.

## Installation
Run the following commands:
```shell
$ clang++ install-sclc.cpp -o install-sclc -std=gnu++17
$ ./install-sclc
```

# Documentation

A list of all features can be found [here](https://stonkdragon.github.io/features.html).

The Scale Framework documentation can be viewed by running the following command:
```shell
$ sclc -doc-for Scale
```

# Build
The install script can also function as the development build script, this requires a full install first:
```shell
$ clang++ install-sclc.cpp -o install-sclc -std=gnu++17
$ ./install-sclc -dev
```

# License

  Scale is licensed under the [MIT license](./LICENSE).
