## fprintf

Arguments:

- arg1: file
    
- arg2: string

Description:

  Prints the string to the file.

Stack Changes:

    Before: 
      arg2 arg1

    After:
      arg1

## fopen

Arguments:

- arg1: string
    
- arg2: string

Description:

  Opens the file with the name arg2 and mode arg1.

Stack Changes:
    
    Before: 
      arg2 arg1
    
    After:
      [fopen(arg1, arg2)]

## fclose

Arguments:

- arg1: file

Description:

  Closes the file.

Stack Changes:
    
    Before: 
      arg1
    
    After:
      <empty>

## fread

Arguments:

- arg1: integer
    
- arg2: file

Description:

  Reads arg1 bytes from the file and pushes the result onto the stack.

Stack Changes:
    
    Before: 
      arg2 arg1
    
    After:
      arg2 [fread(arg1, arg2)]

## fseekstart

Arguments:

- arg1: integer
    
- arg2: file

Description:

  Moves the file pointer to offset arg1 from the start of the file.

Stack Changes:
    
    Before: 
      arg2 arg1
    
    After:
      arg2

## fseekend

Arguments:

- arg1: integer
    
- arg2: file

Description:

  Moves the file pointer to offset arg1 from the end of the file.

Stack Changes:

    Before: 
      arg2 arg1
    
    After:
      arg2

## fseekcur

Arguments:

- arg1: integer
    
- arg2: file

Description:

  Moves the file pointer to offset arg1 from the current position.

Stack Changes:
    
    Before: 
      arg2 arg1
    
    After:
      arg2

## ftell

Arguments:

- arg1: file

Description:

  Pushes the current position of the file onto the stack.

Stack Changes:

    Before:
      arg1

    After:
      arg1 [ftell(arg1)]

## fwrite

Arguments:

- arg1: file
    
- arg2: integer
    
- arg3: string

Description:

  Writes arg2 bytes from arg3 to the file.

Stack Changes:

    Before: 
      arg3 arg2 arg1

    After:
      <empty>

## fileno

Arguments:

- arg1: file

Description:

  Pushes the file descriptor onto the stack.

  Stack Changes:
    
    Before: 
      arg1
    
    After:
      [fileno(arg1)]