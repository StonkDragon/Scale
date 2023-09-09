# Deprecations
## Since Version 23.5
|Function|Replacement|Removed in|
|-|-|-|
|`longToString`|`int::toString`|`23.9`|
|`stringToLong`|`int::parse`|`23.9`|
|`stringToDouble`|`float::toString`|`23.9`|
|`doubleToString`|`float::parse`|`23.9`|
|`nop`|none|`23.9`|
|`toInt8`|`int::toInt8`|`23.9`|
|`toInt16`|`int::toInt16`|`23.9`|
|`toInt32`|`int::toInt32`|`23.9`|
|`toInt`|`int::toInt`|`23.9`|
|`toUInt8`|`int::toUInt8`|`23.9`|
|`toUInt16`|`int::toUInt16`|`23.9`|
|`toUInt32`|`int::toUInt32`|`23.9`|
|`toUInt`|`int::toUInt`|`23.9`|
|`isValidInt8`|`int::isValidInt8`|`23.9`|
|`isValidInt16`|`int::isValidInt16`|`23.9`|
|`isValidInt32`|`int::isValidInt32`|`23.9`|
|`isValidUInt8`|`int::isValidUInt8`|`23.9`|
|`isValidUInt16`|`int::isValidUInt16`|`23.9`|
|`isValidUInt32`|`int::isValidUInt32`|`23.9`|
|`isInfinite`|`float::isInfinite`|`23.9`|
|`isNaN`|`float::isNaN`|`23.9`|
|`isFinite`|`float::isFinite`|`23.9`|
|`isNotNaN`|`float::isNotNaN`|`23.9`|
|`fopen`|`File::new`|`23.9`|
|`fwrite`|`File:writeBinary`|`23.9`|
|`fwrites`|`File:append`|`23.9`|
|`fputs`|`File:puts`|`23.9`|
|`fputint`|none|`23.9`|
|`fputfloat`|none|`23.9`|
|`fread`|`File:read`|`23.9`|
|`fseekstart`|`File:offsetFromBegin`|`23.9`|
|`fseekend`|`File:offsetFromEnd`|`23.9`|
|`fseekcur`|`File:offset`|`23.9`|

## Since Version 23.7
|Function|Replacement|Removed in|
|-|-|-|
|`trace`|`Process::stackTrace`|`23.9`|
|`throwerr`|`Exception`|`23.9`|

## Since Version 23.8
|Function|Replacement|Removed in|
|-|-|-|
|`Array:immutableCopy`|none|`23.9`|
