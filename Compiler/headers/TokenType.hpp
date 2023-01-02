#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

namespace sclc
{
    enum TokenType {
        tok_eof,

        // commands
        tok_return,         // return
        tok_function,       // function
        tok_end,            // end
        tok_true,           // true
        tok_false,          // false
        tok_nil,            // nil
        tok_if,             // if
        tok_then,           // then
        tok_elif,           // elif
        tok_else,           // else
        tok_fi,             // fi
        tok_while,          // while
        tok_do,             // do
        tok_done,           // done
        tok_extern,         // extern
        tok_extern_c,       // extern_c
        tok_break,          // break
        tok_continue,       // continue
        tok_for,            // for
        tok_foreach,        // foreach
        tok_in,             // in
        tok_step,           // step
        tok_to,             // to
        tok_addr_ref,       // addr
        tok_load,           // load
        tok_store,          // store
        tok_declare,        // decl
        tok_container_def,  // container
        tok_repeat,         // repeat
        tok_struct_def,     // struct
        tok_new,            // new
        tok_is,             // is
        tok_cdecl,          // cdecl
        tok_label,          // label
        tok_goto,           // goto
        tok_switch,         // switch
        tok_case,           // case
        tok_esac,           // esac
        tok_default,        // default
        tok_interface_def,  // interface
        tok_as,             // as

        // operators
        tok_question_mark,  // ?
        tok_addr_of,        // @
        tok_paren_open,     // (
        tok_paren_close,    // )
        tok_curly_open,     // {
        tok_curly_close,    // }
        tok_bracket_open,   // [
        tok_bracket_close,  // ]
        tok_comma,          // ,
        tok_add,            // +
        tok_sub,            // -
        tok_mul,            // *
        tok_div,            // /
        tok_mod,            // %
        tok_land,           // &
        tok_lor,            // |
        tok_lxor,           // ^
        tok_lnot,           // ~
        tok_lsh,            // <<
        tok_rsh,            // >>
        tok_pow,            // **
        tok_column,         // :
        tok_double_column,  // ::
        tok_dot,            // .

        tok_identifier,     // foo
        tok_number,         // 123
        tok_number_float,   // 123.456
        tok_string_literal, // "foo"
        tok_char_literal,   // 'a'
    };
} // namespace sclc
