# The Scale Programming Language
## Introduction
  Scale is a [procedual](https://en.wikipedia.org/wiki/Procedural_programming) and [object oriented](https://en.wikipedia.org/wiki/Object-oriented_programming) [concatenative](https://en.wikipedia.org/wiki/Concatenative_programming) [stack oriented](https://en.wikipedia.org/wiki/Stack-oriented_programming) [compiled](https://en.wikipedia.org/wiki/Compiler) programming language inspired by [Lua](https://www.lua.org/) and [Porth](https://gitlab.com/tsoding/porth).

  Scale and [C](https://en.wikipedia.org/wiki/C_(programming_language)) can interoperate using a header file named `scale_support.h`, which will be generated when compiling a scale file with the `-t` option. If you have any scale functions marked with the `export` modifier, C-declarations for those functions will appear in the file. Functions marked with the `expect` keyword are expected to be implemented in a different translation unit, i.e. your C-code.

  The Compiler is a [source-to-source compiler](https://en.wikipedia.org/wiki/Source-to-source_compiler), as it converts your source code to valid C code, that is then compiled by [GCC](https://en.wikipedia.org/wiki/GNU_Compiler_Collection).

  Scale supports both 32-bit and 64-bit systems, but 64-bit is recommended.

## Installation
### Install the latest release
To install the latest release, run the following commands:
```shell
$ git clone https://github.com/StonkDragon/Scale
$ cd Scale
$ sh install.sh
```
Alternatively, if you have `dragon` version 6.0 or newer installed, you can run the following command:
```shell
$ dragon package install StonkDragon/Scale
```

This will install all necessary dependencies and the Scale compiler.

Scale by default will install itself to the `/opt/Scale` directory.

Windows is not supported.

### Install a specific release
Download the source code zip archive from the [releases tab](https://github.com/StonkDragon/Scale/releases) for the release you want to install. Then, extract the archive and run the following command:
```shell
$ sh install.sh
```

Alternatively, if you have `dragon` version 6.0 or newer installed, you can run the following command:
```shell
$ dragon package install StonkDragon/Scale <version-tag>
```

This will install all necessary dependencies and the Scale compiler.

## Dependencies
Scale has the following dependencies:

- `gcc` (or similar C compiler)
- [`dragon`](https://github.com/StonkDragon/Dragon) (installed by [install.sh](./install.sh))
- [`bdwgc`](https://github.com/ivmai/bdwgc) (installed by [install_post.sh](./install_post.sh))

# Documentation

The documentation can be viewed by running the following commands:
```shell
$ sclc -doc categories
$ sclc -doc info
```

## Scale Framework Documentation

The Scale Framework documentation can be viewed by running the following command:
```shell
$ sclc -doc-for Scale
```

## Examples

  Examples can be found in the [examples](./examples) directory.

# Build

  The Scale compiler uses [Dragon](https://github.com/StonkDragon/Dragon) as the build system. It is required to build the compiler.

  To build the compiler, execute the following command from the root directory of the repository:

```shell
$ dragon build
```

# License

  Scale is licensed under the [MIT license](./LICENSE).
