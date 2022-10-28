#include <stdio.h>

#include "scale_support.h"

// Define function `foo` and export it to scale
scl_export(foo) {
    // call scale function `bar`
    bar("Foo!");
}
