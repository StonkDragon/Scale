#ifndef SCALE_SUPPORT_H
#define SCALE_SUPPORT_H

#define scl_export(func_name) \
    void func_name (void); \
    void Function_ ## func_name (void); \
    void Function_ ## func_name () { func_name (); } \
    void func_name (void)

#define ssize_t signed long
#define scl_value void*
#define scl_int long long
#define scl_str char*
#define scl_float double

void bar(scl_value a);
#endif
