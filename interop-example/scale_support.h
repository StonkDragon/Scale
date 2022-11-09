#ifndef SCALE_SUPPORT_H
#define SCALE_SUPPORT_H

#define scl_export(func_name) \
    void func_name (void); \
    void fn_ ## func_name (void); \
    void fn_ ## func_name () { func_name (); } \
    void func_name (void)

#define ssize_t signed long
#define scl_value void*
#define scl_int long long
#define scl_str char*
#define scl_float double

void ctrl_push_string(const scl_str c);
void ctrl_push_long(long long n);
void ctrl_push_double(scl_float d);
void ctrl_push(scl_value n);
scl_str ctrl_pop_string(void);
scl_float ctrl_pop_double(void);
long long ctrl_pop_long(void);
scl_value ctrl_pop(void);

void bar(scl_value a);
#endif
