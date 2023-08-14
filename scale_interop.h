#if !defined(SCALE_INTEROP_H)
#define SCALE_INTEROP_H
#include <scale_runtime.h>

typedef struct Struct_StringIterator* scl_StringIterator;
typedef struct Struct_SclObject* scl_SclObject;
typedef struct Struct_Int* scl_Int;
typedef struct Struct_Exception* scl_Exception;
typedef struct Struct_InvalidSignalException* scl_InvalidSignalException;
typedef struct Struct_NullPointerException* scl_NullPointerException;
typedef struct Struct_Error* scl_Error;
typedef struct Struct_AssertError* scl_AssertError;
typedef struct Struct_CastError* scl_CastError;
typedef struct Struct_UnreachableError* scl_UnreachableError;
typedef struct Struct_Any* scl_Any;
typedef struct Struct_Float* scl_Float;
typedef struct Struct_Union* scl_Union;
typedef struct Struct_Result* scl_Result;
typedef struct Struct_Option* scl_Option;
typedef struct Struct_ConsoleColor* scl_ConsoleColor;
typedef struct Struct_FmtIO* scl_FmtIO;
typedef struct Struct_Array* scl_Array;
typedef struct Struct_ArrayIterator* scl_ArrayIterator;
typedef struct Struct_Process* scl_Process;
typedef struct Struct_ImmutableArray* scl_ImmutableArray;
typedef struct Struct_InvalidArgumentException* scl_InvalidArgumentException;
typedef struct Struct_IndexOutOfBoundsException* scl_IndexOutOfBoundsException;
typedef struct Struct_InvalidTypeException* scl_InvalidTypeException;
typedef struct Struct_TypedArray* scl_TypedArray;
typedef struct Struct_IllegalStateException* scl_IllegalStateException;
typedef struct Struct_ReadOnlyArray* scl_ReadOnlyArray;
typedef struct Struct_Library* scl_Library;
typedef struct Struct_ThreadException* scl_ThreadException;
typedef struct Struct_AtomicOperationException* scl_AtomicOperationException;
typedef struct Struct_Thread* scl_Thread;
struct Struct_str {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_int8* data;
  scl_int length;
  scl_int hash;
};
struct Struct_StringIterator {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str _str;
  scl_int _pos;
};
struct Struct_SclObject {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
};
struct Struct_Int {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_int value;
};
struct Struct_Exception {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_InvalidSignalException {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
  scl_int sig;
};
struct Struct_NullPointerException {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_Error {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_AssertError {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_CastError {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_UnreachableError {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_Any {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_any value;
};
struct Struct_Float {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_float value;
};
struct Struct_Union {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_int __tag;
  scl_any __value;
};
struct Struct_Result {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_int __tag;
  scl_any __value;
};
struct Struct_Option {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_int __tag;
  scl_any __value;
};
struct Struct_Array {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_any* values;
  scl_int count;
  scl_int capacity;
  scl_int initCap;
};
struct Struct_ArrayIterator {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_Array array;
  scl_int pos;
};
struct Struct_ImmutableArray {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_any* values;
  scl_int count;
  scl_int capacity;
  scl_int pos;
};
struct Struct_InvalidArgumentException {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_IndexOutOfBoundsException {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_InvalidTypeException {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_TypedArray {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_any* values;
  scl_int count;
  scl_int capacity;
  scl_int initCap;
  scl_uint32 $template_arg_T;
  scl_int8* $template_argname_T;
};
struct Struct_IllegalStateException {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_ReadOnlyArray {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_any* values;
  scl_int count;
  scl_int capacity;
  scl_int initCap;
};
struct Struct_Library {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_any _handle;
  scl_str _name;
};
struct Struct_ThreadException {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_AtomicOperationException {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  scl_str msg;
  scl_Array stackTrace;
  scl_str errnoStr;
};
struct Struct_Thread {
  const _scl_lambda* const $fast;
  const TypeInfo* $statics;
  mutex_t $mutex;
  _scl_lambda func;
  pthread_t tid;
  scl_str name;
};
extern scl_int8 int8$minValue __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_int8$minValue");
extern scl_int8 int8$maxValue __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_int8$maxValue");
extern scl_int int$minValue __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_int$minValue");
extern scl_int int$maxValue __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_int$maxValue");
extern scl_int float$maxExponent __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_float$maxExponent");
extern scl_int float$minExponent __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_float$minExponent");
extern scl_Array Thread$threads __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_Thread$threads");
extern scl_int Thread$nextID __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_Thread$nextID");
extern scl_Thread Thread$mainThread __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_Thread$mainThread");
extern scl_int builtinIsInstanceOf(scl_any Var_obj, scl_str Var_typeStr) __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "builtinIsInstanceOf");
extern scl_str builtinToString(scl_any Var_val) __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "builtinToString");
#endif
