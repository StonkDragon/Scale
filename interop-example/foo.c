#include <scale_runtime.h>

void bar(scale_str s);

// Define function `foo` and export it to scale
void foo() {

    // creates a new string
    scale_str s = scale_create_string("Foo");

    // calls the `append` method on the string
    s = virtual_call(s, "append(s;)s;", scale_str, scale_create_string("!"));

    // calls the `bar` function defined in `main.scale`
    bar(s);
}
