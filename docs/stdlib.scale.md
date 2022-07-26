## printf

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

Arguments:

    none

Description:

  Reads a line of text from the console.


Stack Changes:

```
  no change
```

## exit

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

Arguments:

    none

Description:

  Pushes the size of the stack onto the stack.


Stack Changes:

```
  no change
```

## sizeof

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


## random

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

## puts

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

## write

Arguments:

- arg1: string
- arg2: integer
- arg3: integer

Description:

  Writes arg2 bytes of string arg1 to file descriptor arg3.

Stack Changes:

```
Before: 
  arg3 arg2 arg1

After:
  <empty>
```

## deref

Arguments:

- arg1: pointer

Description:

  Pushes the value pointed to by arg1 onto the stack.

Stack Changes:

```
Before: 
  arg1

After:
  [*arg1]
```
