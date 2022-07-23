## printf
- Native Function

Arguments:

- arg1: string

Description:

  Prints the string to the console.


Stack Changes:

```
Before:
  arg1

After:
  arg1
```

## read
- Native Function

Arguments:

    none

Description:

  Reads a line of text from the console.


Stack Changes:

```
  no change
```

## exit
- Native Function

Arguments:

- arg1: integer

Description:

  Exits the program with the given exit code.


Stack Changes:

```
Before: 
  arg1

After:
  program has exited
```

## sleep
- Native Function

Arguments:

- arg1: integer

Description:

  Sleeps for the given number of milliseconds.


Stack Changes:

```
Before: 
  arg1

After:
  
```

## eval
- Native Function

Arguments:

- arg1: string

Description:

  Executes the given system command.


Stack Changes:

```
Before: 
  arg1

After:
  exitcode
```

## getproperty
- Native Function

Arguments:

- arg1: string

Description:

  Gets the value of the given system property.


Stack Changes:

```
Before: 
  arg1

After:
  property_value
```

## dup
- Native Function

Arguments:

- arg1: any

Description:

  Duplicates the given value on the stack.


Stack Changes:

```
Before: 
  arg1

After:
  arg1 arg1
```

## over
- Native Function

Arguments:

- arg1: any
    
- arg2: any
    
- arg3: any

Description:

  Swaps the values of arg1 and arg3 on the stack.


Stack Changes:

```
Before: 
  arg3 arg2 arg1

After:
  arg1 arg2 arg3
```

## swap
- Native Function

Arguments:

- arg1: any
    
- arg2: any

Description:

  Swaps the values of arg1 and arg2 on the stack.


Stack Changes:

```
Before: 
  arg2 arg1

After:
  arg1 arg2
```

## drop
- Native Function

Arguments:

- arg1: any

Description:

  Drops the given value from the stack.


Stack Changes:

```
Before: 
  arg1

After:
  <none>
```

## getstack
- Native Function

Arguments:

- arg1: integer

Description:

  Gets the value at the given stack index.


Stack Changes:

```
Before: 
  arg1

After:
  [value on stack at index arg1]
```

## sizeof_stack
- Native Function

Arguments:

    none

Description:

  Pushes the size of the stack onto the stack.


Stack Changes:

```
  no change
```

## sizeof
- Native Function

Arguments:

- arg1: string

Description:

  Pushes the size of the type specified by the string onto the stack.


Stack Changes:

```
Before: 
  arg1

After:
  [size of type specified by arg1]
```

## add
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Adds the two integers and pushes the result onto the stack.


Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg2 + arg1]
```

## sub
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Subtracts arg2 from arg1 and pushes the result onto the stack.


Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg1 - arg2]
```

## mul
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Multiplies the two integers and pushes the result onto the stack.


Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg1 * arg2]
```

## div
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Divides arg1 by the second and pushes the result onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg1 / arg2]
```

## mod
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Divides arg1 by the second and pushes the remainder onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg1 % arg2]
```

## lshift
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Shifts arg1 left by the second and pushes the result onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg1 << arg2]
```

## rshift
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Shifts arg1 right by the second and pushes the result onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg1 >> arg2]
```

## random
- Native Function

Arguments:

    none

Description:

  Pushes a random integer onto the stack.

Stack Changes:

```
Before: 
  <empty>

After:
  [random integer]
```

## less
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Pushes a 1 if arg2 is less than arg1 onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg2 < arg1]
```

## more
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Pushes a 1 if arg2 is more than arg1 onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg2 > arg1]
```

## equal
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Pushes a 1 if arg2 is equal to arg1 onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg2 == arg1]
```

## and
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Pushes 1 if both arg1 and arg2 are 1 onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg2 && arg1]
```

## not
- Native Function

Arguments:

- arg1: integer

Description:

  Pushes 1 if arg1 is 0 onto the stack.

Stack Changes:

```
Before: 
  arg1

After:
  [!arg1]
```

## or
- Native Function

Arguments:

- arg1: integer
    
- arg2: integer

Description:

  Pushes 1 if either arg1 or arg2 is 1 onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [arg2 || arg1]
```

## sprintf
- Native Function

Arguments:

- arg1: string
    
- arg2: string

Description:

  Pushes a formatted string onto the stack, where arg1 contains the format string and arg2 contains the value to format.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [formatted string]
```

## strlen
- Native Function

Arguments:

- arg1: string

Description:

  Pushes the length of the string onto the stack.

Stack Changes:

```
Before: 
  arg1

After:
  [length of arg1]
```

## tochars
- Native Function

Arguments:

- arg1: string

Description:

  Pushes the string as a list of characters onto the stack.

Stack Changes:

```
Before: 
  arg1

After:
  [characters of arg1 individually]
```

## strcmp
- Native Function

Arguments:

- arg1: string
    
- arg2: string

Description:

  Pushes a 1 if arg1 is equal to arg2 onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [strcmp(arg1, arg2)]
```

## strncmp
- Native Function

Arguments:

- arg1: string
    
- arg2: string
    
- arg3: integer

Description:

  Pushes a 1 if arg1 is equal to arg2 onto the stack.

Stack Changes:

```
Before: 
  arg3 arg2 arg1

After:
  [strncmp(arg1, arg2, arg3)]
```

## concat
- Native Function

Arguments:

- arg1: string
    
- arg2: string

Description:

  Pushes the concatenation of arg1 and arg2 onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [strcat(arg1, arg2)]
```

## strstarts
- Native Function

Arguments:

- arg1: string
    
- arg2: string

Description:

  Pushes a 1 if arg1 starts with arg2 onto the stack.

Stack Changes:

```
Before: 
  arg2 arg1

After:
  [strncmp(arg1, arg2, strlen(arg2))]
```

## putstr

Arguments:

- arg1: string

Description:

  Prints the string and a newline "\n" to the screen.

Stack Changes:

```
Before: 
  arg1

After:
  arg1
```

## putint

Arguments:

- arg1: integer

Description:

  Prints the integer and a newline "\n" to the screen.

Stack Changes:

```
Before: 
  arg1

After:
  ["%d\n" (%d is arg1)]
```

## clearstack

Arguments:

    none

Description:

  Clears the stack.

Stack Changes:

```
Before: 
  any

After:
  <empty>
```

## swap2

Arguments:

- arg1: any
    
- arg2: any
    
- arg3: any

Description:

  Swaps the values of arg2 and arg3.

Stack Changes:
    
```
Before: 
  arg3 arg2 arg1

After:
  arg2 arg3 arg1
```

## sdup2

Arguments:

- arg1: any
    
- arg2: any

Description:

  Duplicates the value of arg2.

Stack Changes:
    
```
Before: 
  arg2 arg1

After:
  arg2 arg1 arg2
```

## throwerr

Arguments:

- arg1: string

Description:

  Throws an error with the string and exits the program with exit code 1.

Stack Changes:

```
Before: 
  arg1

After:
  <empty>
```

## nop

Arguments:

    none

Description:

  Does nothing.

Stack Changes:
    
```
Before: 
  any

After:
  any
```
