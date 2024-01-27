#include "scale_interop.h"

// Define function `foo` and export it to scale
void foo() {

    // creates a new string
    scl_str s = _scl_create_string("Foo");

    // calls the `append` method on the string
    s = virtual_call(s, "append(s;)s;", _scl_create_string("!"));

    // calls the `bar` function defined in `main.scale`
    bar(s);
}
