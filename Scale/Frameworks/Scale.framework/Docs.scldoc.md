# Scale Standard Library
## Any
### final static struct any
The static struct `any` provides utility functions for working with the `any` type.

### static function any::new(sz: int): any
Returns a pointer to a newly allocated memory block of size `sz` bytes.

### static function any::delete(ptr: any): none
Frees the memory block pointed to by `ptr`.

### static function any::size(ptr: any): int
Returns the size of the memory block pointed to by `ptr`.
Returns -1 if `ptr` is nil or invalid.

### static function any::resize(newSize: int, ptr: any): any
Resizes the memory block pointed to by `ptr` to `newSize` bytes.
Returns a pointer to the resized memory block.

### static function any::default(): any
Returns a pointer to a newly allocated memory block of size 8 bytes.

### static function any::toString(x: any): str
Returns a string representation of `x`.

### static function any::toHexString(x: any): str
Returns a hexadecimal string representation of `x`.

### static function any::referenceEquals(one: any, other: any): bool
Returns true if `one` and `other` point to the same memory block.

### static function any::asReference(x: any): Any
Returns a new instance of `Any` that represents the value `x`.

### struct Any
The struct `Any` is the reference type that represents the `any` type.
Declared Fields:
- `value: readonly any` - The value of the `Any` instance.

### static function Any::valueOf(x: any): Any
Returns a new instance of `Any` that represents the value `x`.

### function Any:equals(other: any): bool
Returns true if `self` and `other` point to the same memory block.

### function Any:==(other: any): bool
Returns true if `self` and `other` point to the same memory block.

### function Any:@(): any
Returns the data pointed to by this `Any` instance.

### function Any:toString(): str
Returns a string representation of this `Any` instance.

## Core
### unsafe function memset(ptr: [any], val: int32, len: int): [any]
Sets `len` bytes of memory pointed to by `ptr` to `val` and returns `ptr`.
This function is unsafe because it does not check if `ptr` is nil or invalid.

### unsafe function memcpy(dst: [any], src: [any], n: int): [any]
Copies `n` bytes from `src` to `dst` and returns `dst`.
This function is unsafe because it does not check if `dst` or `src` are nil or invalid.

### function sizeofStack(): int
Returns the size of the stack in elements.

### function sleep(millis: int): none
Sleeps for `millis` milliseconds.

### function malloc(size: int): [any]
Allocates a memory block of `size` bytes and returns a pointer to it.

### function free(pointer: [any]): none
Frees the memory block pointed to by `pointer`.

### function realloc(pointer: [any], size: int): [any]
Resizes the memory block pointed to by `pointer` to `size` bytes and returns a pointer to it.

### function system(command: str): int
Executes `command` in the system shell and returns the exit code.

### function getenv(environKey: str): str?
Returns the value of the environment variable `environKey` or nil if it is not set.

### function time(): float
Returns the current time in seconds since the epoch.

### function longToString(theNumber: int): str
Returns a string representation of `theNumber`.

### function stringToLong(theString: str): int
Returns an integer representation of `theString`.

### function stringToDouble(theString: str): float
Parses `theString` as a float and returns the result.

### function doubleToString(theNumber: float): str
Returns a string representation of `theNumber`.

### function nop(): none
Does nothing.

## Union
### struct Union
The struct `Union` is the base struct of all `union` types.
Declared Fields:
- `__tag: readonly int` - The tag of the union.
- `__value: readonly any` - The data of the union.

Do not use this struct directly, it is only used internally.
Do not access the fields of this struct directly if you do not know what you are doing.

### static function Union::new(tag: int, value: any): Union
Returns a new instance of `Union` with the given `tag` and `value`.

### function Union:toString(): str
Returns a string representation of this `Union` instance.

### function Union:expected(id: int): none
Asserts that the tag of this `Union` instance is equal to `id`.

### function Union:==(other: any): bool
Returns true if `self` and `other` represent the same union and have the same tag and value.

### function Union:!=(other: any): bool
Returns true if `self` and `other` represent different unions or have different tags or values.

## Result
### union Result
The union `Result` represents a value or an error.
Declared Fields:
- `Ok: any` - The `Ok` value of the `Result` instance.
- `Err: any` - The `Err` value of the `Result` instance.

Accessing the `Ok` or `Err` field of a `Result` instance that does not represent the corresponding value throws an `AssertError`.

### static function Result::Ok(value: any): Result
Returns a new instance of `Result` with the `Ok` value `value`.

### static function Result::Err(value: any): Result
Returns a new instance of `Result` with the `Err` value `value`.

### function Result:isOk(): bool
Returns true if this `Result` instance represents an `Ok` value.

### function Result:isErr(): bool
Returns true if this `Result` instance represents an `Err` value.

### function Result:toString(): str
Returns a string representation of this `Result` instance.

## Option
### union Option
The union `Option` represents a value or nothing.
Declared Fields:
- `Some: any` - The `Some` value of the `Option` instance.
- `None: none` - The `None` value of the `Option` instance.

Accessing the `Some` field of an `Option` instance that does not contain a value throws an `AssertError`.

### static function Option::Some(value: any): Option
Returns a new instance of `Option` with the `Some` value `value`.

### static function Option::None(): Option
Returns a new instance of `Option` with the `None` value.

### function Option:isSome(): bool
Returns true if this `Option` instance represents a `Some` value.

### function Option:isNone(): bool
Returns true if this `Option` instance represents a `None` value.

### function Option:toString(): str
Returns a string representation of this `Option` instance.

## Debug
### function trace(): none
Prints a stack trace to stdout.

### function throw(ex: Exception): nothing
Throws the given exception `ex`.

### function errno(): int
Returns the value of the errno variable.

### function strerror(): str
Returns a string representation of the error code in errno.

### function setSignalHandler(fun: lambda(int): none, sig: int): none
Sets the signal handler for the signal `sig` to `fun`.

### function resetSignalHandler(sig: int): none
Resets the signal handler for the signal `sig` to the default handler.

### function dumpStack(): none
Prints the current stack to stdout.

### function crash(): none
Crashes the program.

### function raise(sigNum: int): none
Raises the signal `sigNum`.

### function breakPoint(): none
Sets a breakpoint.

### struct Exception
The struct `Exception` is the base class for all exceptions.
Declared Fields:
- `msg: str` - The message of the exception.
- `stackTrace: const Array` - The stack trace of the exception.
- `errnoStr: readonly str` - The string representation of the error code in errno at the time the exception was initialized.

### static function Exception::new(): Exception
Returns a new instance of `Exception`.

### function Exception:printStackTrace(): none
Prints the stack trace of this exception to stdout.

### final struct InvalidSignalException: Exception
The struct `InvalidSignalException` is thrown when an invalid signal is passed to `setSignalHandler` or `resetSignalHandler`.
Declared Fields:
- `sig: readonly int` - The signal number that was passed to `setSignalHandler` or `resetSignalHandler`.

### static function InvalidSignalException::new(sig: int): InvalidSignalException
Returns a new instance of `InvalidSignalException` with the given signal number `sig`.

### struct nilPointerException: Exception
The struct `nilPointerException` is thrown when a nil pointer is dereferenced.

### struct Error: Exception
The struct `Error` is the base class for all errors.
Errors are thrown when the program encounters an unrecoverable error and cannot be handled by the program.

### static function Error::new(msg: str): Error
Returns a new instance of `Error` with the given message `msg`.

### final struct AssertError: Error
The struct `AssertError` is thrown when an assertion fails.

### final struct CastError: Error
The struct `CastError` is thrown when a cast fails.

### final struct UnreachableError: Error
The struct `UnreachableError` is thrown when the program encounters a statement that is unreachable.

### function sysPrettyString(): str
Returns a string representation of the system.

## Filesystem
### final struct File
The struct `File` represents a file on the filesystem.

### static function File::new(name: str): File
Returns a new instance of `File` with the given name `name`.

### function File:name(): str
Returns the name of this file.

### function File:close(): none
Closes this file. Any further operations on this file will result in undefined behavior.

### function File:append(s: str): none
Appends the string `s` to this file.

### function File:puts(s: any): none
Writes the string representation of `s` to this file.

### function File:writeBinary(buf: any, sz: int): none
Writes `sz` bytes from the memory block pointed to by `buf` to this file.

### function File:read(sz: int): any
Reads `sz` bytes from this file and returns a pointer to the memory block containing the data.

### function File:offsetFromBegin(offset: int): none
Sets the offset of this file to `offset` bytes from the beginning of the file.

### function File:offsetFromEnd(offset: int): none
Sets the offset of this file to `offset` bytes from the end of the file.

### function File:offset(offset: int): none
Sets the offset of this file to `offset` bytes from the current offset.

### function File:fileno(): int
Returns the file descriptor of this file.

### function File:pos(): int
Returns the current offset of this file.

### function File:delete(): bool
Deletes this file and returns true if the operation was successful.

### function File:create(): none
Creates this file.

### static function File::exists(f: str): bool
Returns true if the file with the name `f` exists.

### unsafe function fopen(fileName: str, mode: str): __c_file?
Opens the file with the name `fileName` in the mode `mode` and returns a pointer to the file.
This function is unsafe as it does not check for nil on its arguments.
This function is deprecated and should not be used.

### unsafe function fwrite(buffer: any, elemSize: int, filePtr: __c_file): none
Writes `elemSize` bytes from the memory block pointed to by `buffer` to the file pointed to by `filePtr`.
This function is unsafe as it does not check for nil on its arguments.
This function is deprecated and should not be used.

### unsafe function fwrites(buffer: str, elemSize: int, filePtr: __c_file): none
Writes `elemSize` bytes from the memory block pointed to by `buffer` to the file pointed to by `filePtr`.
This function is unsafe as it does not check for nil on its arguments.
This function is deprecated and should not be used.

### unsafe function fputs(filePtr: __c_file, _str_: str): none
Writes the string `_str_` to the file pointed to by `filePtr`.
This function is unsafe as it does not check for nil on its arguments.
This function is deprecated and should not be used.

### unsafe function fputint(filePtr: __c_file, _int_: int): none
Writes the integer `_int_` to the file pointed to by `filePtr`.
This function is unsafe as it does not check for nil on its arguments.
This function is deprecated and should not be used.

### unsafe function fputfloat(filePtr: __c_file, _float_: float): none
Writes the float `_float_` to the file pointed to by `filePtr`.
This function is unsafe as it does not check for nil on its arguments.
This function is deprecated and should not be used.

### unsafe function fread(filePtr: __c_file, elemSize: int): [any]?
Reads `elemSize` bytes from the file pointed to by `filePtr` and returns a pointer to the memory block containing the data.
This function is unsafe as it does not check for nil on its arguments.
This function is deprecated and should not be used.

### unsafe function fseekstart(filePtr: __c_file, offset: int): none
Seek to `offset` bytes from the beginning of the file pointed to by `filePtr`.
This function is unsafe as it does not check for nil on its arguments.
This function is deprecated and should not be used.

### unsafe function fseekend(filePtr: __c_file, offset: int): none
Seek to `offset` bytes from the end of the file pointed to by `filePtr`.
This function is unsafe as it does not check for nil on its arguments.
This function is deprecated and should not be used.

### unsafe function fseekcur(filePtr: __c_file, offset: int): none
Seek to `offset` bytes from the current offset of the file pointed to by `filePtr`.
This function is unsafe as it does not check for nil on its arguments.
This function is deprecated and should not be used.

## Float
### final static struct float
The static struct `float` provides utility functions for working with the `float` type.
Static Fields:
- `maxExponent: const int` - The maximum exponent of a float.
- `minExponent: const int` - The minimum exponent of a float.

### static function float::toString(x: float): str
Returns a string representation of `x`.

### static function float::toPrecisionString(x: float): str
Returns a string representation of `x` with a precision of 17.

### static function float::toHexString(x: float): str
Returns a hexadecimal string representation of `x`.

### static function float::parse(s: str): float
Parses `s` as a float and returns the result.

### static function float::bits(x: float): int
Returns the bits of `x`.

### static function float::isInfinite(x: float): bool
Returns true if `x` is infinite.

### static function float::isNaN(x: float): bool
Returns true if `x` is NaN.

### static function float::isFinite(x: float): bool
Returns true if `x` is finite.

### static function float::isNotNaN(x: float): bool
Returns true if `x` is not NaN.

### static function float::asReference(f: float): Float
Returns a new instance of `Float` that represents the value `f`.

### static function float::fromBits(bits: any): float
Returns a float from the bits `bits`.

### struct Float
The struct `Float` is the reference type that represents the `float` type.
Declared Fields:
- `value: readonly float` - The value of the `Float` instance.

### static function Float::valueOf(x: float): Float
Returns a new instance of `Float` that represents the value `x`.

### function Float:equals(other: any): bool
Returns true if `self` and `other` represent the same value.

### function Float:==(other: any): bool
Returns true if `self` and `other` represent the same value.

### function Float:@(): float
Returns the float value of this `Float` instance.

### function Float:toString(): str
Returns a string representation of this `Float` instance.

## IO
### unsafe function write(fileDescriptor: int, string: str): none
Writes the string `string` to the file with the file descriptor `fileDescriptor`.

### function read(fileDescriptor: int, amount: int): str
Reads `amount` bytes from the file with the file descriptor `fileDescriptor` and returns a string containing the data.

### function getchar(): int8
Reads a character from stdin and returns it.

### function puts(val: str | [int8] | float | any): none
Writes the string representation of `val` to stdout.

### function eputs(s: str | [int8] | float | any): none
Writes the string representation of `s` to stderr.

### function putint(number: int): none
Writes the string representation of `number` to stdout.

### function putfloat(number: float): none
Writes the string representation of `number` to stdout.

### function throwerr(errorString: str): none 
This function has been deprecated in version 23.7 and should be replaced with Exception
Throws an Exception with the message `errorString`.

### final static struct ConsoleColor
The static struct `ConsoleColor` provides utility functions for working with the console color.

### static function ConsoleColor::reset(): str
Returns the ANSI escape sequence to reset the console color.

### static function ConsoleColor::black(): str
Returns the ANSI escape sequence to set the console color to black.

### static function ConsoleColor::red(): str
Returns the ANSI escape sequence to set the console color to red.

### static function ConsoleColor::green(): str
Returns the ANSI escape sequence to set the console color to green.

### static function ConsoleColor::yellow(): str
Returns the ANSI escape sequence to set the console color to yellow.

### static function ConsoleColor::blue(): str
Returns the ANSI escape sequence to set the console color to blue.

### static function ConsoleColor::magenta(): str
Returns the ANSI escape sequence to set the console color to magenta.

### static function ConsoleColor::cyan(): str
Returns the ANSI escape sequence to set the console color to cyan.

### static function ConsoleColor::white(): str
Returns the ANSI escape sequence to set the console color to white.

### static function ConsoleColor::bold(): str
Returns the ANSI escape sequence to set the console color to bold.

### final static struct FmtIO
The static struct `FmtIO` provides utility functions for working with formatted output.

### static function FmtIO::puts(fmt: str, args: varargs): none
Writes the formatted string `fmt` to stdout.

## Int
### final static struct int8
The static struct `int8` provides utility functions for working with the `int8` type.

### static function int8::toString(x: int8): str
Returns a string representation of `x`.

### final static struct int
The static struct `int` provides utility functions for working with the `int` type.

### static function int::toString(x: int): str
Returns a string representation of `x`.

### static function int::toHexString(x: int): str
Returns a hexadecimal string representation of `x`.

### static function int::parse(s: str): int
Parses `s` as an int and returns the result.

### static function int::toInt8(value: any): int8
Cast `value` to an int8.

### static function int::toInt16(value: any): int16
Cast `value` to an int16.

### static function int::toInt32(value: any): int32
Cast `value` to an int32.

### static function int::toInt(value: any): int
Cast `value` to an int.

### static function int::toUInt8(value: any): uint8
Cast `value` to a uint8.

### static function int::toUInt16(value: any): uint16
Cast `value` to a uint16.

### static function int::toUInt32(value: any): uint32
Cast `value` to a uint32.

### static function int::toUInt(value: any): uint
Cast `value` to a uint.

### static function int::isValidInt8(val: any): bool
Returns true if `val` can be safely cast to an int8 without loss of precision.

### static function int::isValidInt16(val: any): bool
Returns true if `val` can be safely cast to an int16 without loss of precision.

### static function int::isValidInt32(val: any): bool
Returns true if `val` can be safely cast to an int32 without loss of precision.

### static function int::isValidUInt8(val: any): bool
Returns true if `val` can be safely cast to a uint8 without loss of precision.

### static function int::isValidUInt16(val: any): bool
Returns true if `val` can be safely cast to a uint16 without loss of precision.

### static function int::isValidUInt32(val: any): bool
Returns true if `val` can be safely cast to a uint32 without loss of precision.

### static function int::asReference(i: int): Int
Returns a new instance of `Int` that represents the value `i`.

### struct Int
The struct `Int` is the reference type that represents the `int` type.
Declared Fields:
- `value: readonly int` - The value of the `Int` instance.

### static function Int::valueOf(x: int): Int
Returns a new instance of `Int` that represents the value `x`.

### function Int:equals(other: any): bool
Returns true if `self` and `other` represent the same value.

### function Int:==(other: any): bool
Returns true if `self` and `other` represent the same value.

### function Int:@(): int
Returns the int value of this `Int` instance.

### function Int:toString(): str
Returns a string representation of this `Int` instance.

### function toInt8(value: any): int8 
This function has been deprecated in version 23.5 and should be replaced with int::toInt8
Cast `value` to an int8.

### function toInt16(value: any): int16 
This function has been deprecated in version 23.5 and should be replaced with int::toInt16
Cast `value` to an int16.

### function toInt32(value: any): int32 
This function has been deprecated in version 23.5 and should be replaced with int::toInt32
Cast `value` to an int32.

### function toInt(value: any): int 
This function has been deprecated in version 23.5 and should be replaced with int::toInt
Cast `value` to an int.

### function toUInt8(value: any): uint8 
This function has been deprecated in version 23.5 and should be replaced with int::toUInt8
Cast `value` to a uint8.

### function toUInt16(value: any): uint16 
This function has been deprecated in version 23.5 and should be replaced with int::toUInt16
Cast `value` to a uint16.

### function toUInt32(value: any): uint32 
This function has been deprecated in version 23.5 and should be replaced with int::toUInt32
Cast `value` to a uint32.

### function toUInt(value: any): uint 
This function has been deprecated in version 23.5 and should be replaced with int::toUInt
Cast `value` to a uint.

### function isValidInt8(val: any): bool 
This function has been deprecated in version 23.5 and should be replaced with int::isValidInt8
Returns true if `val` can be safely cast to an int8 without loss of precision.

### function isValidInt16(val: any): bool 
This function has been deprecated in version 23.5 and should be replaced with int::isValidInt16
Returns true if `val` can be safely cast to an int16 without loss of precision.

### function isValidInt32(val: any): bool 
This function has been deprecated in version 23.5 and should be replaced with int::isValidInt32
Returns true if `val` can be safely cast to an int32 without loss of precision.

### function isValidUInt8(val: any): bool 
This function has been deprecated in version 23.5 and should be replaced with int::isValidUInt8
Returns true if `val` can be safely cast to a uint8 without loss of precision.

### function isValidUInt16(val: any): bool 
This function has been deprecated in version 23.5 and should be replaced with int::isValidUInt16
Returns true if `val` can be safely cast to a uint16 without loss of precision.

### function isValidUInt32(val: any): bool 
This function has been deprecated in version 23.5 and should be replaced with int::isValidUInt32
Returns true if `val` can be safely cast to a uint32 without loss of precision.

### function isnil(x: any): bool
Returns true if `x` is nil.

### function isnotnil(x: any): bool
Returns true if `x` is not nil.

## Math
### final static struct MathConstants
The static struct `MathConstants` provides constants for mathematical operations.
Declared Fields:
- `e: const float` - The mathematical constant e.
- `pi: const float` - The mathematical constant pi.
- `nan: const float` - The floating point representation of NaN.
- `infinity: const float` - The floating point representation of infinity.
- `negativeInfinity: const float` - The floating point representation of negative infinity.

### function acos(:float): float
Returns the arc cosine of `x`.

### function asin(:float): float
Returns the arc sine of `x`.

### function atan(:float): float
Returns the arc tangent of `x`.

### function atan2(:float, :float): float
Returns the arc tangent of `y/x` in the range [-pi, pi].

### function cos(:float): float
Returns the cosine of `x`.

### function sin(:float): float
Returns the sine of `x`.

### function tan(:float): float
Returns the tangent of `x`.

### function acosh(:float): float
Returns the inverse hyperbolic cosine of `x`.

### function asinh(:float): float
Returns the inverse hyperbolic sine of `x`.

### function atanh(:float): float
Returns the inverse hyperbolic tangent of `x`.

### function cosh(:float): float
Returns the hyperbolic cosine of `x`.

### function sinh(:float): float
Returns the hyperbolic sine of `x`.

### function tanh(:float): float
Returns the hyperbolic tangent of `x`.

### function exp(:float): float
Returns the base-e exponential of `x`.

### function exp2(:float): float
Returns the base-2 exponential of `x`.

### function expm1(:float): float
Returns the base-e exponential of `x` minus 1.

### function log(:float): float
Returns the natural logarithm of `x`.

### function log10(:float): float
Returns the base-10 logarithm of `x`.

### function log2(:float): float
Returns the base-2 logarithm of `x`.

### function log1p(:float): float
Returns the natural logarithm of `x` plus 1.

### function logb(:float): float
Returns the exponent of `x`.

### function fabs(:float): float
Returns the absolute value of `x`.

### function cbrt(:float): float
Returns the cube root of `x`.

### function hypot(:float, :float): float
Returns the square root of the sum of the squares of `x` and `y`.

### function pow(:float, :float): float
Returns `x` raised to the power of `y`.

### function sqrt(:float): float
Returns the square root of `x`.

### function erf(:float): float
Returns the error function of `x`.

### function erfc(:float): float
Returns the complementary error function of `x`.

### function lgamma(:float): float
Returns the natural logarithm of the absolute value of the gamma function of `x`.

### function tgamma(:float): float
Returns the gamma function of `x`.

### function ceil(:float): float
Returns the smallest integer value greater than or equal to `x`.

### function floor(:float): float
Returns the largest integer value less than or equal to `x`.

### function nearbyint(:float): float
Returns the integer value nearest to `x`.

### function rint(:float): float
Returns the integer value nearest to `x`.

### function round(:float): float
Rounds `x` to the nearest integer value.

### function trunc(:float): float
Round `x` to the nearest integer not larger in magnitude.

### function fmod(:float, :float): float
Returns the floating-point remainder of `x/y`.

### function remainder(:float, :float): float
Returns the remainder of `x/y`.

### function fdim(:float, :float): float
Returns the positive difference between `x` and `y`.

### function fmax(:float, :float): float
Returns the maximum of `x` and `y`.

### function fmin(:float, :float): float
Returns the minimum of `x` and `y`.

### function fma(:float, :float, :float): float
Returns `x*y+z`.

### function isInfinite(x: float): bool 
This function has been deprecated in version 23.5 and should be replaced with float::isInfinite
Returns true if `x` is infinite.

### function isNaN(x: float): bool 
This function has been deprecated in version 23.5 and should be replaced with float::isNaN
Returns true if `x` is NaN.

### function isFinite(x: float): bool 
This function has been deprecated in version 23.5 and should be replaced with float::isFinite
Returns true if `x` is finite.

### function isNotNaN(x: float): bool 
This function has been deprecated in version 23.5 and should be replaced with float::isNotNaN
Returns true if `x` is not NaN.

### function random(): int
Returns a random integer.

### function iseven(number: int): int
Returns true if `number` is even.

### function isodd(number: int): int
Returns true if `number` is odd.

### function lrandom(limit: int): int
Returns a random integer in the range 0 to `limit` (exclusive).

### function lerp(a: float, b: float, t: float): float
Returns the linear interpolation of `a` and `b` at `t`.

### function neg(number: int): int
Returns the negation of `number`.

## SclObject
### struct SclObject is Cloneable, Equatable, Hashable
The struct `SclObject` is the base class for all objects in Scale.

### function SclObject:==(other: any): bool
Returns true if the hash codes of `self` and `other` are equal.

### function SclObject:equals(other: any): bool
Returns true if the hash codes of `self` and `other` are equal.

### function SclObject:clone(): SclObject
Returns a new instance of `SclObject` that is a clone of `self`.

### function SclObject:hashCode(): int
Returns the hash code of `self`.

### function SclObject:lock(): none
Locks `self` on the current thread, blocking if another thread has already locked `self`.

### function SclObject:unlock(): none
Unlocks `self` on the current thread.

### static function SclObject::new(): SclObject
Returns a new instance of `SclObject`.

## String
### final struct str is Equatable, Cloneable, Stringifyable, Iterable
The struct `str` represents a string.

### function str:+(what: str | int | float | [int8]): str
Prepends `what` to `self` and returns the result.

### function str:clone(): str
Returns a new instance of `str` that is a clone of `self`.

### function str:append(what: str | [int8]): str
Appends `what` to `self` and returns the result.

### function str:prepend(what: str | [int8]): str
Prepends `what` to `self` and returns the result.

### function str:equals(other: any): bool
Returns true if `self` and `other` represent the same string.

### function str:==(other: any): bool
Returns true if `self` and `other` represent the same string.

### function str:!=(other: any): bool
Returns true if `self` and `other` do not represent the same string.
Returns false if `other` is not a string.

### function str:size(): int
Returns the size of `self`.

### function str:at(i: int): int
Returns the character at index `i` in `self`.

### function str:toString(): str
Returns `self`.

### function str:@(i: int): int
Returns the character at index `i` in `self`.

### function str:[](index: int): int8
Returns the character at index `index` in `self`.

### function str:reverse(): str
Returns a reversed copy of `self`.

### function str:toChars(): Array
Returns an array containing the characters of `self`.

### function str:split(delimiter: str): Array
Splits `self` at `delimiter` and returns an array containing the parts.

### function str:startsWith(string: str): bool
Returns true if `self` starts with `string`.

### function str:endsWith(string: str): bool
Returns true if `self` ends with `string`.

### function str:subString(start: int): str
Returns a substring of `self` starting at `start`.

### function str:view(): [int8]
Returns a pointer to the memory block containing the data of `self`.

### static function str::new(data: [int8]): str
Returns a new instance of `str` with the data `data`.

### function str:iterate(): StringIterator
Returns an iterator over the characters of `self`.

### function str:hashCode(): int
Returns the hash code of `self`.

### static function str::=>(data: [int8]): str
Allows assigning C strings to `str` variables.

### final struct StringIterator is Iterator
The struct `StringIterator` is an iterator over the characters of a string.

### static function StringIterator::new(s: str): StringIterator
Returns a new instance of `StringIterator` that iterates over the characters of `s`.

### function StringIterator:next(): int8
Returns the next character in the string.

### function StringIterator:hasNext(): bool
Returns true if there are more characters to iterate over.

### function +(string: str, number: int | float): str
Appends the string representation of `number` to `string` and returns the result.

## Fraction
### final struct Fraction
The struct `Fraction` represents a fraction.
Declared Fields:
- `numerator: int` - The numerator of the fraction.
- `denominator: int` - The denominator of the fraction.

### static function Fraction::new(numerator: int, denominator: int): Fraction
Returns a new instance of `Fraction` with the numerator `numerator` and the denominator `denominator`.

### function Fraction:toString(): str
Returns a string representation of this `Fraction` instance.

### function Fraction:toFloat(): float
Returns the float value of this `Fraction` instance.

### function Fraction:+(other: Fraction): Fraction
Returns the sum of `self` and `other`.

### function Fraction:-(other: Fraction): Fraction
Returns the difference of `self` and `other`.

### function Fraction:++(): Fraction
Returns the sum of `self` and 1.

### function Fraction:--(): Fraction
Returns the difference of `self` and 1.

### function Fraction:*(other: Fraction): Fraction
Returns the product of `self` and `other`.

### function Fraction:inverse(): Fraction
Returns the inverse of `self`.

### function Fraction:/(other: Fraction): Fraction
Returns the quotient of `self` and `other`.

### function Fraction:isNan(): bool
Returns true if `self` is NaN.

### function Fraction:isInfinite(): bool
Returns true if `self` is infinite.

### function Fraction:isFinite(): bool
Returns true if `self` is finite.

### function Fraction:isNotNan(): bool
Returns true if `self` is not NaN.

### static function Fraction::fromFloat(d: float): Fraction
Returns a fraction from the float `d`.

## Runtime: GarbageCollector
### final static struct GarbageCollector
The static struct `GarbageCollector` provides utility functions for working with the garbage collector.

### static function GarbageCollector::heapSize(): int
Returns the size of the heap in bytes.

### static function GarbageCollector::freeBytesEstimate(): int
Returns the number of bytes that are free on the heap.

### static function GarbageCollector::bytesSinceLastCollect(): int
Returns the number of bytes allocated since the last garbage collection.

### static function GarbageCollector::totalMemory(): int
Returns the total amount of memory allocated by the garbage collector.

### static function GarbageCollector::collect(): none
Performs a garbage collection.

### static function GarbageCollector::pause(): none
Pauses the garbage collector.

### static function GarbageCollector::resume(): none
Resumes the garbage collector.

### static function GarbageCollector::isPaused(): bool
Returns true if the garbage collector is paused.

### static function GarbageCollector::implementation(): str
Returns the name of the garbage collector implementation.

## Runtime: Library
### final struct Library
The struct `Library` represents a shared library.

### static function Library::new(name: str): Library
Returns a new instance of `Library` with the name `name`.

### function Library:close(): none
Closes this library.

### function Library:name(): str
Returns the name of this library.

### function Library:getSymbol(name: str): any
Returns the symbol with the name `name` from this library.

## Runtime: Process
### final static struct Process
The static struct `Process` provides utility functions for working with the current process.

### static function Process::exit(:int): none
Exits the current process with the exit code `code`.

### static function Process::stackTrace(): Array
Returns an array containing the current stack trace.

### static function Process::loadLibrary(name: str): Library
Loads the shared library with the name `name` and returns it.

### static function Process::gcEnabled(): bool
Returns true if the garbage collector is enabled.

### static function Process::isOnMainThread(): bool
Returns true if the current thread is the main thread.

### static function Process::lock(obj: SclObject): none
Locks `obj` on the current thread, blocking if another thread has already locked `obj`.

### static function Process::unlock(obj: SclObject): none
Unlocks `obj` on the current thread.

### static function Process::stackPointer(): [any]
Returns the stack pointer of the current thread.

### static function Process::basePointer(): [any]
Returns the stack base pointer of the current thread.

## Thread
### final struct ThreadException: Exception
The struct `ThreadException` represents an exception that is thrown when an error occurs in a thread.

### static function ThreadException::new(msg: str): ThreadException
Returns a new instance of `ThreadException` with the message `msg`.

### final struct AtomicOperationException: Exception
The struct `AtomicOperationException` represents an exception that is thrown when an error occurs in an atomic operation.

### static function AtomicOperationException::new(msg: str): AtomicOperationException
Returns a new instance of `AtomicOperationException` with the message `msg`.

### final struct Thread
The struct `Thread` represents a thread.
Declared Fields:
- `name: str` - The name of the thread.

Static Fields:
- `mainThread: readonly Thread` - The main thread.

### static function Thread::stackTrace(): Array
Returns an array containing stack trace of the current thread.

### static function Thread::currentThread(): Thread
Returns the current thread.

### static function Thread::new(func: lambda(): none): Thread
Returns a new instance of `Thread` that executes `func`.

### function Thread:start(): none
Starts this thread.

### function Thread:join(): none
Waits for this thread to finish.

### function Thread:equals(other: any): bool
Returns true if `self` and `other` represent the same thread.

### function Thread:==(other: any): bool
Returns true if `self` and `other` represent the same thread.

## Util: Array
### struct Array is Iterable
The struct `Array` represents an array.
Declared Fields:
- `values: readonly [any]` - The values of the array.
- `count: readonly int` - The number of values in the array.
- `capacity: readonly int` - The capacity of the array.
- `initCap: readonly int` - The initial capacity of the array.

### function Array:sort(): none
Sorts the array.

### function Array:[](index: int): any
Returns the value at index `index` in `self`.
Throws an `IndexOutOfBoundsException` if `index` is out of bounds.

### function Array:get(index: int): any
Returns the value at index `index` in `self`.
Throws an `IndexOutOfBoundsException` if `index` is out of bounds.

### function Array:=>[](index: int, value: any): none
Sets the value at index `index` in `self` to `value`.
Throws an `IndexOutOfBoundsException` if `index` is out of bounds.

### function Array:set(index: int, value: any): none
Sets the value at index `index` in `self` to `value`.
Throws an `IndexOutOfBoundsException` if `index` is out of bounds.

### function Array:top(): any
Returns the last value in `self`.
Throws an `IndexOutOfBoundsException` if `self` is empty.

### function Array:push(value: any): none
Appends `value` to `self`.

### function Array:pop(): none
Removes the last value from `self`.

### function Array:indexOf(val: any): int
Returns the index of `val` in `self` or -1 if `val` is not in `self`.

### function Array:contains(val: any): bool
Returns true if `val` is in `self`.

### static function Array::new(size: int): Array
Returns a new instance of `Array` with the size `size`.

### function Array:iterate(): ArrayIterator
Returns an iterator over the values of `self`.

### function Array:reverse(): Array
Reverses the order of the values in `self` and returns the result.

### function Array:toString(): str
Returns a string representation of `self`.

### function Array:remove(val: any): none
Removes the first occurrence of `val` from `self`.

### function Array:removeAll(val: any): none
Removes all occurrences of `val` from `self`.

### function Array:map(x: lambda(any): none): none
Applies `x` to each value in `self`.

### function Array:filter(x: lambda(any): bool): Array
Returns a new array containing the values of `self` for which `x` returns true.

### function Array:clone(): Array
Returns a new instance of `Array` that is a clone of `self`.

### function Array:*(): none
Unpacks the values of `self` onto the stack.

### static function Array::fromPointerCollection(count: int, values: [any]): Array
Returns a new instance of `Array` with the values `values`.

### struct ArrayIterator is Iterator
The struct `ArrayIterator` is an iterator over the values of an array.

### static function ArrayIterator::new(arr: Array): ArrayIterator
Returns a new instance of `ArrayIterator` that iterates over the values of `arr`.

### function ArrayIterator:hasNext(): bool
Returns true if there are more values to iterate over.

### function ArrayIterator:next(): any
Returns the next value in the array.

## Util: Cloneable
### interface Cloneable
The interface `Cloneable` is implemented by all types that can be cloned.

### function Cloneable:clone(): ?
Returns a new instance of the type that is a clone of `self`.

## Util: Equatable
### interface Equatable
The interface `Equatable` is implemented by all types that can be compared for equality.

### function Equatable:==(other: any): bool
Returns true if `self` and `other` represent the same value.

### function Equatable:equals(other: any): bool
Returns true if `self` and `other` represent the same value.

## Util: Hashable
### interface Hashable
The interface `Hashable` is implemented by all types that can be hashed.

### function Hashable:hashCode(): int
Returns the hash code of `self`.

## Util: Iterable
### interface Iterator
The interface `Iterator` is implemented by all types that implement some kind of iteration.
See also: `interface Iterable`

### function Iterator:hasNext(): bool
Returns true if there are more values to iterate over.

### function Iterator:next(): ?
Returns the next value in the iteration.

### function Iterable:iterate(): Iterator
Returns an iterator over the values of `self`.

### interface Iterable
The interface `Iterable` is implemented by all types that can be iterated over.

## Util: Stringifyable
### interface Stringifyable
The interface `Stringifyable` is implemented by all types that can be converted to a string.

### function Stringifyable:toString(): str
Returns a string representation of `self`.

## Util: IndexOutOfBoundsException
### final struct IndexOutOfBoundsException: Exception
The struct `IndexOutOfBoundsException` represents an exception that is thrown when an index is out of bounds.

### static function IndexOutOfBoundsException::new(s: str): IndexOutOfBoundsException
Returns a new instance of `IndexOutOfBoundsException` with the message `s`.

## Util: InvalidArgumentException
### final struct InvalidArgumentException: Exception
The struct `InvalidArgumentException` represents an exception that is thrown when an invalid argument is passed to a function.

### static function InvalidArgumentException::new(s: str): InvalidArgumentException
Returns a new instance of `InvalidArgumentException` with the message `s`.

## Util: Map
### final struct MapEntry<T: any>
The struct `MapEntry` represents an entry in a map.
Declared Fields:
- `key: str` - The key of the entry.
- `value: T` - The value of the entry.

### static function MapEntry<T: any>::new(key: str, value: T): MapEntry
Returns a new instance of `MapEntry` with the key `key` and the value `value`.

### function MapEntry:toString(): str
Returns a string representation of this `MapEntry` instance.

### struct Map<T: any> is Iterable
The struct `Map` represents a map.

### static function Map<T: any>::new(size: int): Map
Returns a new instance of `Map` with the size `size`.

### function Map:[](key: str): T
Returns the value for the key `key` in `self`.
Throws an `InvalidArgumentException` if `key` is not in `self`.

### function Map:get(key: str): T
Returns the value for the key `key` in `self`.
Throws an `InvalidArgumentException` if `key` is not in `self`.

### function Map:=>[](key: str, value: T): none
Sets the value for the key `key` in `self` to `value`.

### function Map:containsKey(key: str): bool
Returns true if `key` is in `self`.

### function Map:set(key: str, value: T): none
Sets the value for the key `key` in `self` to `value`.

### function Map:iterate(): Iterator
Returns an iterator over the entries of `self`.

### function Map:toString(): str
Returns a string representation of `self`.

### function Map:map(x: lambda(MapEntry): none): none
Applies `x` to each entry in `self`.

### struct MapIterator is Iterator
The struct `MapIterator` is an iterator over the entries of a map.

### static function MapIterator::new(data: Array): MapIterator
Returns a new instance of `MapIterator` that iterates over the entries of `data`.

### function MapIterator:hasNext(): bool
Returns true if there are more entries to iterate over.

### function MapIterator:next(): MapEntry
Returns the next entry in the map.

## Util: Pair
### final struct Pair
The struct `Pair` represents a pair of values.
Declared Fields:
- `a: any` - The first value.
- `b: any` - The second value.

### static function Pair::new(a: any, b: any): Pair
Returns a new instance of `Pair` with the values `a` and `b`.

### function Pair:toString(): str
Returns a string representation of this `Pair` instance.

## Util: Range
### final struct Range is Iterable
The struct `Range` represents a range of integers.
Declared Fields:
- `lower: int` - The lower bound of the range.
- `upper: int` - The upper bound of the range.

### static function Range::new(theStart: int, theEnd: int): Range
Returns a new instance of `Range` with the lower bound `theStart` and the upper bound `theEnd`.

### function Range:containsRange(other: Range): int
Returns true if `other` is contained in `self`.

### function Range:overlaps(other: Range): int
Returns true if `other` overlaps with `self`.

### function Range:contains(pos: int): int
Returns true if `pos` is contained in `self`.

### function Range:toString(): str
Returns a string representation of this `Range` instance.

### function Range:iterate(): Iterator
Returns an iterator over the values of `self`.

### function Range:toReverseRange(): ReverseRange
Returns a reversed copy of `self`.

### final struct ReverseRange is Iterable
The struct `ReverseRange` represents a reversed range of integers.
Declared Fields:
- `lower: int` - The lower bound of the range.
- `upper: int` - The upper bound of the range.

### static function ReverseRange::new(theStart: int, theEnd: int): ReverseRange
Returns a new instance of `ReverseRange` with the lower bound `theStart` and the upper bound `theEnd`.

### function ReverseRange:toString(): str
Returns a string representation of this `ReverseRange` instance.

### function ReverseRange:iterate(): Iterator
Returns an iterator over the values of `self`.

### function ReverseRange:toRange(): Range
Returns a copy of `self`.

### struct RangeIterator is Iterator
The struct `RangeIterator` is an iterator over the values of a range.

### static function RangeIterator::new(theRange: Range): RangeIterator
Returns a new instance of `RangeIterator` that iterates over the values of `theRange`.

### function RangeIterator:hasNext(): bool
Returns true if there are more values to iterate over.

### function RangeIterator:next(): int
Returns the next value in the range.

### struct ReverseRangeIterator is Iterator
The struct `ReverseRangeIterator` is an iterator over the values of a reversed range.

### static function ReverseRangeIterator::new(theRange: ReverseRange): ReverseRangeIterator
Returns a new instance of `ReverseRangeIterator` that iterates over the values of `theRange`.

### function ReverseRangeIterator:hasNext(): bool
Returns true if there are more values to iterate over.

### function ReverseRangeIterator:next(): int
Returns the next value in the range.

## Util: ReadOnlyArray
### final struct IllegalStateException: Exception
The struct `IllegalStateException` represents an exception that is thrown when an illegal state is encountered.

### static function IllegalStateException::new(msg: str): IllegalStateException
Returns a new instance of `ReadOnlyArray` with the message `msg`.

### struct ReadOnlyArray: Array
The struct `ReadOnlyArray` represents an array that cannot be modified.

### function ReadOnlyArray:sort(): none
Throws an `IllegalStateException`.

### function ReadOnlyArray:set(index: int, value: any): none
Throws an `IllegalStateException`.

### function ReadOnlyArray:push(value: any): none
Throws an `IllegalStateException`.

### function ReadOnlyArray:pop(): none
Throws an `IllegalStateException`.

### function ReadOnlyArray:reverse(): Array
Throws an `IllegalStateException`.

### function ReadOnlyArray:remove(val: any): none
Throws an `IllegalStateException`.

### function ReadOnlyArray:removeAll(val: any): none
Throws an `IllegalStateException`.

### static function ReadOnlyArray::fromArray(arr: Array): ReadOnlyArray
Returns a new instance of `ReadOnlyArray` with the values of `arr`.

### static function ReadOnlyArray::fromPointerCollection(count: int, values: [any]): ReadOnlyArray
Returns a new instance of `ReadOnlyArray` with the values `values`.

## Util: Triple
### final struct Triple
The struct `Triple` represents a triple of values.

### static function Triple::new(a: any, b: any, c: any): Triple
Returns a new instance of `Triple` with the values `a`, `b` and `c`.

### function Triple:toString(): str
Returns a string representation of this `Triple` instance.

## Util: TypedArray
### final struct InvalidTypeException: Exception
The struct `InvalidTypeException` represents an exception that is thrown when an invalid type is encountered.

### static function InvalidTypeException::new(msg: str): InvalidTypeException
Returns a new instance of `InvalidTypeException` with the message `msg`.

### final struct TypedArray<T: any>: Array
The struct `TypedArray` represents an array that can only contain values of a certain type.

### static function TypedArray<T: any>::new(size: int): TypedArray
Returns a new instance of `TypedArray` with the size `size`.