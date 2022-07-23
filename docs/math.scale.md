## dadd

Arguments:
    arg1: float
    arg2: float

Description:
    Pushes the sum of arg1 and arg2 onto the stack.

Stack Changes:
    
    Before: 
      arg2 arg1

    After:
      [arg1 + arg2]

## dsub

Arguments:
    arg1: float
    arg2: float

Description:
    Subtracts arg2 from arg1 and pushes the result onto the stack.

Stack Changes:

    Before: 
      arg2 arg1

    After:
      [arg1 - arg2]

## dmul

Arguments:
    arg1: float
    arg2: float

Description:
    Multiplies arg1 and arg2 and pushes the result onto the stack.

Stack Changes:
        
    Before: 
      arg2 arg1
    
    After:
      [arg1 * arg2]

## ddiv

Arguments:
    arg1: float
    arg2: float

Description:
    Divides arg1 by arg2 and pushes the result onto the stack.

Stack Changes:

    Before: 
      arg2 arg1
    
    After:
      [arg1 / arg2]

## dtoi

Arguments:
    arg1: float

Description:
    Casts arg1 to an integer and pushes the result onto the stack.

Stack Changes:

    Before: 
      arg1
    
    After:
      [(int) arg1]

## itod

Arguments:
    arg1: integer

Description:
    Casts arg1 to a float and pushes the result onto the stack.

Stack Changes:
 
    Before: 
      arg1
    
    After:
      [(float) arg1]

## iseven

Arguments:
    arg1: integer

Description:
    Pushes a 1 if arg1 is even onto the stack.

Stack Changes:
    
    Before: 
      arg1

    After:
      [arg1 % 2 == 0]

## inc

Arguments:
    arg1: integer

Description:
    Increments arg1 and pushes the result onto the stack.

Stack Changes:
    
    Before: 
      arg1
    
    After:
      [arg1 + 1]

## dec

Arguments:
    arg1: integer

Description:
    Decrements arg1 and pushes the result onto the stack.

Stack Changes:

    Before: 
      arg1
    
    After:
      [arg1 - 1]

## lrandom

Arguments:
    arg1: integer

Description:
    Pushes a random integer between 0 and arg1-1 onto the stack.

Stack Changes:

    Before: 
      arg1

    After:
      [rand() % arg1]
