#ifndef SCALE_H
#define SCALE_H

void scale_push_string(char* c);
void scale_push_int(int n);
void scale_push_long(long long n);
void scale_push(void* n);
char* scale_pop_string();
long long scale_pop_long();
long long scale_pop_int();

int scale_extern_trace();
int scale_extern_printf();
int scale_extern_read();
int scale_extern_tochars();
int scale_extern_exit();
int scale_extern_sleep();
int scale_extern_getproperty();
int scale_extern_less();
int scale_extern_more();
int scale_extern_equal();
int scale_extern_dup();
int scale_extern_over();
int scale_extern_swap();
int scale_extern_drop();
int scale_extern_sizeof_stack();
int scale_extern_concat();
int scale_extern_strstarts();
int scale_extern_add();
int scale_extern_sub();
int scale_extern_mul();
int scale_extern_div();
int scale_extern_mod();
int scale_extern_lshift();
int scale_extern_rshift();
int scale_extern_random();
int scale_extern_crash();
int scale_extern_and();
int scale_extern_eval();
int scale_extern_getstack();
int scale_extern_not();
int scale_extern_or();

int scale_extern_sprintf();
int scale_extern_strlen();
int scale_extern_strcmp();
int scale_extern_strncmp();

int scale_extern_fprintf();
int scale_extern_fopen();
int scale_extern_fclose();
int scale_extern_fread();
int scale_extern_fseekstart();
int scale_extern_fseekend();
int scale_extern_fseekcur();
int scale_extern_ftell();
int scale_extern_fwrite();
int scale_extern_fgetc();
int scale_extern_fputc();
int scale_extern_fgets();
int scale_extern_fputs();
int scale_extern_fgetpos();
int scale_extern_fsetpos();

#endif // SCALE_H
