#ifdef __cplusplus
extern "C" {
#endif
#include <scale.h>

/* FUNCTION PROTOTYPES */

/* FUNCTION HEADERS */
void fun_puts(scl_word $str);
void fun_eputs(scl_word $str);
void fun_putint(scl_word $int);
void fun_throwerr(scl_word $err);
void fun_clearstack();
void fun_swap2(scl_word $c, scl_word $b, scl_word $a);
void fun_sdup2(scl_word $b, scl_word $a);
void fun_dup(scl_word $a);
void fun_swap(scl_word $b, scl_word $a);
void fun_drop(scl_word $a);
void fun_over(scl_word $c, scl_word $b, scl_word $a);
void fun_nop();
void fun_strstarts(scl_word $s2, scl_word $s1);
void fun_sizeof(scl_word $type);
void fun_toChar(scl_word $val);
void fun_toInt(scl_word $val);
void fun_toShort(scl_word $val);
void fun_toChars(scl_word $str);
void fun_main();

/* EXTERNS */
void native_write();
void native_read();
void native_sprintf();
void native_exit();
void native_sleep();
void native_system();
void native_getenv();
void native_sizeof_stack();
void native_malloc();
void native_free();
void native_less();
void native_more();
void native_equal();
void native_and();
void native_not();
void native_or();
void native_strlen();
void native_strcmp();
void native_strncmp();
void native_concat();
void native_strrev();
void native_time();

/* FUNCTIONS */
void fun_puts(scl_word $str) {
    function_start("puts(str)");
    push_long(1);
    push($str);
    function_native_start("strlen");
    native_strlen();
    function_native_end();
    push($str);
    function_native_start("write");
    native_write();
    function_native_end();
    push_long(1);
    push_long(1);
    push_string("\n");
    function_native_start("write");
    native_write();
    function_native_end();
    function_end();
}

void fun_eputs(scl_word $str) {
    function_start("eputs(str)");
    push_long(2);
    push($str);
    function_native_start("strlen");
    native_strlen();
    function_native_end();
    push($str);
    function_native_start("write");
    native_write();
    function_native_end();
    push_long(2);
    push_long(1);
    push_string("\n");
    function_native_start("write");
    native_write();
    function_native_end();
    function_end();
}

void fun_putint(scl_word $int) {
    function_start("putint(int)");
    push($int);
    push_string("%lld");
    function_native_start("sprintf");
    native_sprintf();
    function_native_end();
    require_elements(1, "puts");
    fun_puts(pop());
    function_end();
}

void fun_throwerr(scl_word $err) {
    function_start("throwerr(err)");
    push($err);
    require_elements(1, "puts");
    fun_puts(pop());
    push_long(1);
    function_native_start("exit");
    native_exit();
    function_native_end();
    function_end();
}

void fun_clearstack() {
    function_nps_start("clearstack()");
    while (1) {
        function_native_start("sizeof_stack");
        native_sizeof_stack();
        function_native_end();
        push_long(0);
        function_native_start("more");
        native_more();
        function_native_end();
        if (!pop_long()) break;
        require_elements(1, "drop");
        fun_drop(pop());
    }
    function_nps_end();
}

void fun_swap2(scl_word $c, scl_word $b, scl_word $a) {
    function_nps_start("swap2(a, b, c)");
    push($b);
    push($a);
    push($c);
    function_nps_end();
}

void fun_sdup2(scl_word $b, scl_word $a) {
    function_nps_start("sdup2(a, b)");
    push($a);
    push($b);
    push($a);
    function_nps_end();
}

void fun_dup(scl_word $a) {
    function_nps_start("dup(a)");
    push($a);
    push($a);
    function_nps_end();
}

void fun_swap(scl_word $b, scl_word $a) {
    function_nps_start("swap(a, b)");
    push($b);
    push($a);
    function_nps_end();
}

void fun_drop(scl_word $a) {
    function_nps_start("drop(a)");
    function_nps_end();
}

void fun_over(scl_word $c, scl_word $b, scl_word $a) {
    function_nps_start("over(a, b, c)");
    push($c);
    push($b);
    push($a);
    function_nps_end();
}

void fun_nop() {
    function_start("nop()");
    push_long(0);
    function_native_start("sleep");
    native_sleep();
    function_native_end();
    function_end();
}

void fun_strstarts(scl_word $s2, scl_word $s1) {
    function_start("strstarts(s1, s2)");
    push($s2);
    function_native_start("strlen");
    native_strlen();
    function_native_end();
    push($s2);
    push($s1);
    function_native_start("strncmp");
    native_strncmp();
    function_native_end();
    scl_word return_value = pop();
    function_end();
    push(return_value);
}

void fun_sizeof(scl_word $type) {
    function_start("sizeof(type)");
    push($type);
    push_string("int");
    function_native_start("strcmp");
    native_strcmp();
    function_native_end();
    if (pop_long()) {
        push_long(4);
        scl_word return_value = pop();
        function_end();
        push(return_value);
        return;
    }
    push($type);
    push_string("long");
    function_native_start("strcmp");
    native_strcmp();
    function_native_end();
    if (pop_long()) {
        push_long(8);
        scl_word return_value = pop();
        function_end();
        push(return_value);
        return;
    }
    push($type);
    push_string("long long");
    function_native_start("strcmp");
    native_strcmp();
    function_native_end();
    if (pop_long()) {
        push_long(8);
        scl_word return_value = pop();
        function_end();
        push(return_value);
        return;
    }
    push($type);
    push_string("float");
    function_native_start("strcmp");
    native_strcmp();
    function_native_end();
    if (pop_long()) {
        push_long(4);
        scl_word return_value = pop();
        function_end();
        push(return_value);
        return;
    }
    push($type);
    push_string("double");
    function_native_start("strcmp");
    native_strcmp();
    function_native_end();
    if (pop_long()) {
        push_long(8);
        scl_word return_value = pop();
        function_end();
        push(return_value);
        return;
    }
    push($type);
    push_string("char");
    function_native_start("strcmp");
    native_strcmp();
    function_native_end();
    if (pop_long()) {
        push_long(1);
        scl_word return_value = pop();
        function_end();
        push(return_value);
        return;
    }
    push($type);
    push_string("size_t");
    function_native_start("strcmp");
    native_strcmp();
    function_native_end();
    if (pop_long()) {
        push_long(8);
        scl_word return_value = pop();
        function_end();
        push(return_value);
        return;
    }
    push($type);
    push_string("ssize_t");
    function_native_start("strcmp");
    native_strcmp();
    function_native_end();
    if (pop_long()) {
        push_long(8);
        scl_word return_value = pop();
        function_end();
        push(return_value);
        return;
    }
    push_long(-1);
    scl_word return_value = pop();
    function_end();
    push(return_value);
}

void fun_toChar(scl_word $val) {
    function_start("toChar(val)");
    push($val);
    push_long(255);
    op_land();
    scl_word return_value = pop();
    function_end();
    push(return_value);
}

void fun_toInt(scl_word $val) {
    function_start("toInt(val)");
    push($val);
    push_long(2147483647);
    op_land();
    scl_word return_value = pop();
    function_end();
    push(return_value);
}

void fun_toShort(scl_word $val) {
    function_start("toShort(val)");
    push($val);
    push_long(32767);
    op_land();
    scl_word return_value = pop();
    function_end();
    push(return_value);
}

void fun_toChars(scl_word $str) {
    function_nps_start("toChars(str)");
    scl_word $len;
    push($str);
    function_native_start("strlen");
    native_strlen();
    function_native_end();
    $len = pop();
    scl_word $i;
    push($len);
    $i = pop();
    while (1) {
        push($i);
        push_long(0);
        function_native_start("more");
        native_more();
        function_native_end();
        push($i);
        push_long(0);
        function_native_start("equal");
        native_equal();
        function_native_end();
        function_native_start("or");
        native_or();
        function_native_end();
        if (!pop_long()) break;
        push($str);
        push($i);
        op_add();
        push(*(scl_word*) pop());
        require_elements(1, "toChar");
        fun_toChar(pop());
        push($i);
        push_long(1);
        op_sub();
        $i = pop();
    }
    function_nps_end();
}

void fun_main() {
    function_start("main()");
