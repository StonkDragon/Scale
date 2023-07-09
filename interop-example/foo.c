#include <scale_runtime.h>

void bar(scl_str str);

// Define function `foo` and export it to scale
void foo() {

    // creates a new string
    scl_str s = str_of("Foo");

    // calls the `append` method on the string
    s = virtual_call(s, "append(s;)s;", str_of("!"));

    // calls the `bar` function
    bar(s);
}
