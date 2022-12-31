# Scale Framework Documentation
## Contents
- [Core](#core)
- [IO](#io)
- [Math](#math)
- [File](#file)
- [Array](#array)
- [Pair](#pair)
- [Triple](#triple)
- [Ranges](#ranges)

<div style="page-break-after: always;"></div>

## [Core](./include/core.scale)
### `function sleep(_millis_: int): none`
Sleeps for `_millis_` milliseconds.

### `function system(_cmd_: str): int`
Executes the shell command `_cmd_` and returns the return value. If `_cmd_` is `nil` the function will return a non-zero value if a command processor was found.

### `function getenv(_key_: str): str`
Returns the value of the environment variable `_key_`. If the environment variable is not set, `nil` is returned. Modifying the returned string is undefined behavior.

### `function sizeof_stack(): int`
Returns the current stack size.

### `function malloc(_size_: int): any`
Returns a pointer to `_size_` bytes of memory. This memory may not be `0`-initialized. It is undefined behavior to pass in a `_size_` of `0`.

### `function realloc(_size_: int, _ptr_: any): any`
Reallocates the memory pointed to by `_ptr_` with a new size of `_size_` bytes. Any call to `realloc` may invalidate the old pointer so it is strongly suggested to reassign it to the return value. If `_ptr_` is `nil` the function will behave identical to `malloc`. It is undefined behavior to pass in a `_size_` of `0`.

### `function free(_ptr_: any): none`
Frees the memory pointed to by `_ptr_`. If `_ptr_` was not allocated, a call to `free` will result in undefined behavior. If `_ptr_` is `nil`, the function will do nothing.

### `function strlen(_str_: str): int`
Returns the length of the string `_str_`. It is undefined behavior if `_str_` is `nil`.

### `function strsplit(_str_: str, _delim_: str): Array`
Splits the string `_str_` at every occurence of `_delim_` and returns an array containing the results. Returns an empty array if the string does not contain `_delim_`.

Example:
```scale
"hello world!" " " strsplit
```
Will split the string 'hello world!' at every space character, returning an array with these elements:
1. "hello"
2. "world!"

### `function strcmp(_str1_: str, _str2_: str): int`
Returns `true`, if `_str1_` and `_str2_` are equal, `false` otherwise.

### `function strncmp(_str1_: str, _str2_: str, _n_: int): int`
Returns `true`, if `_str1_` and `_str2_` are equal, `false` otherwise. The function will only compare up to `_n_` characters until a character differs, or until a `nil` byte is found.

### `function concat(_str1_: str, _str2_: str): str`
Concates `_str1_` and `_str2_` together.

### `function strrev(_str_: str): str`
Reverses `_str_`. Returns an empty string if `_str_` has a length of zero.

### `function time(): float`
Pushes the current time in seconds as a float onto the stack.

### `function longToString(_long_: int): str`
Converts `_long_` to a string.

### `function stringToLong(_str_: str): int`
Converts `_str_` to an interger. Returns `0` if the string is not a valid number. If `_str_` contains the string representation of a float, this function will return the whole part of the number.

### `function stringToDouble(_str_: str): float`
Converts `_str_` to a float. Returns `0.0` if the string is not a valid float. Undefined behavior is invoked if the number is out of range for a 64-Bit floating point number.

### `function doubleToString(_double_: float): str`
Converts `_double_` to a string.

### `function nop(): none`
Do nothing.

### `function strstarts(_s1_: str, _s2_: str): int`
Returns `true`, if `_s1_` starts with `_s2_`

### `function sizeof(_type_: str): int`
Returns the size of `_type_`. Returns `-1` if `_type_` is not recognized.

### `function toChar(_val_: int): int`
Converts `_val_` to an 8-Bit integer.

### `function toShort(_val_: int): int`
Converts `_val_` to a 16-Bit integer.

### `function toInt(_val_: int): int`
Converts `_val_` to a 32-Bit integer.

### `function toChars(_str_: str): Array`
Converts `_str_` to an array of characters. An empty array is returned if the string has a length of zero.

<div style="page-break-after: always;"></div>

## [IO](./include/io.scale)
### `function write(_fd_: int, _str_: str, _n_: int): none`
Writes `_n_` bytes of `_str_` to file descriptor `_fd_`.

### `function read(_fd_: int, _n_: int): any`
Reads `_n_` bytes from file descriptor `_fd_`.

### `function puts(_str_: str): none`
Prints `_str_` followed by a new line to the standard output.

### `function eputs(_str_: str): none`
Prints `_str_` followed by a new line to the standard error output.

### `function putint(_int_: int): none`
Prints the number `_int_` followed by a new line to the standard output.

### `function putfloat(_f_: float): none`
Prints the float `_f_` followed by a new line to the standard output.

### `function throwerr(_err_: str): none`
Prints `_err_` to standard error output and exits with exit code 1.

<div style="page-break-after: always;"></div>

## [Math](./include/math.scale)
### `container MathConstants`
Member types:
- `e: float`
- `pi: float`

`e`: The math constant `e`. It is equal to `2.71828182845904523536028747135266250`

`pi`: The math constant `pi`. It is equal to `3.14159265358979323846264338327950288`

### `function sqrt(_f_: float): float`
Calculates the square root of `_f_`.

### `function sin(_f_: float): float`
Calculates the sine of `_f_`.

### `function cos(_f_: float): float`
Calculates the cosine of `_f_`.

### `function tan(_f_: float): float`
Calculates the tangent of `_f_`.

### `function asin(_f_: float): float`
Calculates the arc sine of `_f_`.

### `function acos(_f_: float): float`
Calculates the arc cosine of `_f_`.

### `function atan(_f_: float): float`
Calculates the arc tangent of `_f_`.

### `function atan2(_n1_: float, _n2_: float): float`
Calculates the arc tangent of an argument.

### `function sinh(_f_: float): float`
Calculates the hyperbolic sine of `_f_`.

### `function cosh(_f_: float): float`
Calculates the hyperbolic cosine of `_f_`.

### `function tanh(_f_: float): float`
Calculates the hyperbolic tangent of `_f_`.

### `function asinh(_f_: float): float`
Calculates the arc hyperbolic sine of `_f_`.

### `function acosh(_f_: float): float`
Calculates the arc hyperbolic cosine of `_f_`.

### `function atanh(_f_: float): float`
Calculates the arc hyperbolic tangent of `_f_`.

### `function exp(_f_: float): float`
Calculates `MathConstants.e` raised to the power of `_f_`.

### `function log(_f_: float): float`
Calculates the natural logarithm of `_f_`.

### `function log10(_f_: float): float`
Calculates the base 10 logarithm of `_f_`.

### `function random(): int`
Generates a random number.

### `function iseven(_i_: int): int`
Pushes `true` if `_i_` is even.

### `function isodd(_i_: int): int`
Pushes `true` if `_i_` is odd.

### `function lrandom(_limit_: int): int`
Pushes a random number not exceeding `_limit_`.

### `function lerp(_a_: int, _b_: int, _t_: int): int`
Linear interpolation between `_a_` and `_b_` with the argument `_t_`.

### `function neg(_i_: int): int`
Returns `_i_ * -1`. `neg` is equivalent to `_i_ -1 *`.

<div style="page-break-after: always;"></div>

## [File](./include/file.scale)
### `function fopen(_filename_: str, _mode_: str): any`
Opens a new file handle of file `_filename_` in mode `_mode_`.

### `function fclose(_file_: any): none`
Closes the file handle `_file_`.

### `function ftell(_file_: any): int`
Returns the current position in the file `_file_`.

### `function fileno(_file_: any): int`
Returns the file descriptor of `_file_`.

### `function fseek(_file_: any, _whence_: int, _offset_: int): none`
Seeks to `_offset_` in the `_file_` from `_whence_`.

### `function fwrite(_buf_: any, _size_: int, _file_: any): none`
Writes `_size_` bytes of buffer `_buf_` to the file `_file_`.

### `function fputs(_file_: any, _str_: str): none`
Prints the string `_str_` to `_file_`.

### `function fread(_f_: any, _size_: int): any`
Reads `_size_` bytes from file `_f_`.

### `function fseekstart(_f_: any, _offset_: int): none`
Seeks to position `_offset_` from the start of file `_f_`.

### `function fseekend(_f_: any, _offset_: int): none`
Seeks to position `_offset_` from the end of file `_f_`.

### `function fseekcur(_f_: any, _offset_: int): none`
Seeks to position `_offset_` from the current position of file `_f_`.

<div style="page-break-after: always;"></div>

## [Array](./include/util/array.scale)
### `struct Array`
Member types:
- `values: any`
- `count: int`
- `capacity: int`

`values`: Contains the contents of the array.

`count`: Contains the amount of elements in the array.

`capacity`: Contains the maximum capacity of the array.

<<RED>>Modifying any of these values will result in undefined behaviour!<<RESET>>

### `function Array:sort(): none`
Sorts the array. If the array is empty, the function will do nothing.

### `function Array:get(index: int): any`
Pushes the value at `index` onto the stack. It is undefined behavior to access indices greater than or equal to `count`

### `function Array:set(index: int, value: any): none`
Sets the value at `index` to `value`. It is undefined behavior to access indices greater than or equal to `count`

### `function Array:top(): any`
Returns the top value of the array. This invocation is identical to `Array.count 1 - Array:get`. It is undefined behavior to use `Array:top` on an empty array.

### `function Array:push(value: any): none`
Pushes `value` onto the array. If the array has no more free slots, it will be resized accordingly.

### `function Array:pop(): none`
Pops the top value of the array by decrementing the `count` property on the array. Invoking `Array:pop` on an empty array is undefined behavior.

### `function Array:contains(val: any): bool`
Returns `true` if the array contains val.

### `static function Array::new(size: int): none`
Create an empty array with an initial capacity of `size` elements, where `size` is bigger than `0`.

<div style="page-break-after: always;"></div>

## [Pair](./include/util/pair.scale)
### `static function Pair::new(a: any, b: any): none`
Create a new pair of `a` and `b`.

<div style="page-break-after: always;"></div>

## [Triple](./include/util/triple.scale)

### `static function Triple::new(a: any, b: any, c: any): none`
Create a new triple of `a`, `b` and `c`.

<div style="page-break-after: always;"></div>

## [Ranges](./include/util/range.scale)
### `static function Range::new(_start: int, _end: int): none`
Create a new Range object from `_start` to `_end` counting up.

### `function Range:containsRange(other: Range): int`
Returns `true` if `other` lies completely within the range. Returns `true` if both ranges are equal.

### `function Range:overlaps(other: Range): int`
Returns `true` if `other` is partially contained within the range. For example, a Range from 2 to 7 will overlap with a Range from 4 to 10.

### `function Range:contains(pos: int): int`
Returns `true` if `pos` is contained within the range. For example, on a Range from 1 to 5, this method will return `true` for all numbers from 1 to 5, both including.

### `function Range:toReverseRange(): ReverseRange`
Convert range to a `ReverseRange` object. For example, a range from 1 to 5 will return a `ReverseRange` object from 5 to 1.

### `static function ReverseRange::new(_start: int, _end: int): none`
Create a new ReverseRange object from `_start` to `_end` counting down.

### `function ReverseRange:toRange(): Range`
Convert reverse range to a `Range` object. For example, a reverse range from 5 to 1 will return a `Range` object from 1 to 5.
