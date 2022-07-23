
## dumpstack

Arguments:
    none

Description:
    Prints the stack to stdout.

Stack Changes:
    
    none

## trace

Arguments:
    none

Description:
    Prints the callstack to stdout.

Stack Changes:
    
    none

## crash

Arguments:
    none

Description:
    Crashes the program with exit code 1.

Stack Changes:
        
    none


## raise

Arguments:
    arg1: integer

Description:
    Raises the signal arg1.

Stack Changes:
    
    Before: 
      arg1

    After:
      <empty>

## abort

Arguments:
    none

Description:
    Raises the signal SIGABRT.

Stack Changes:
    
    none

