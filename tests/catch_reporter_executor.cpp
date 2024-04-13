// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// this diagnostic has to be turned off "globally" so the catch2 does not
// generate the warning on the floating point == operator
//
#pragma GCC diagnostic ignored "-Wfloat-equal"

// self
//
#include    "catch_main.h"


// reporter
//
#include    <eventdispatcher/reporter/executor.h>

#include    <eventdispatcher/reporter/parser.h>
#include    <eventdispatcher/reporter/variable_address.h>
#include    <eventdispatcher/reporter/variable_floating_point.h>
#include    <eventdispatcher/reporter/variable_integer.h>
#include    <eventdispatcher/reporter/variable_list.h>
#include    <eventdispatcher/reporter/variable_string.h>
#include    <eventdispatcher/reporter/variable_timestamp.h>
#include    <eventdispatcher/reporter/variable_void.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{



// `call()` -- DONE
// `exit()` -- PARTIAL
// `goto()` -- DONE
// `if()` -- DONE
// `label()` -- DONE
// `return()` -- DONE
// `run()` -- DONE
// `sleep()` -- DONE

constexpr char const * const g_program_sleep_func =
    "call(label: func_sleep_1s)\n"
    "exit(timeout: 1)\n"
    "label(name: func_sleep_1s)\n"
    "sleep(seconds: 2.5)\n"
    "return()\n"
;

constexpr char const * const g_program_start_thread =
    "set_variable(name: test, value: 33)\n"
    "run()\n"
    "set_variable(name: runner, value: 6.07)\n"
;

constexpr char const * const g_program_start_thread_twice =
    "set_variable(name: test, value: 33)\n"
    "run()\n"
    "set_variable(name: runner, value: 6.07)\n"
    "run()\n" // second run() is forbidden
;

constexpr char const * const g_program_verify_computation_integer =
    "set_variable(name: t01, value: 3)\n"

    "set_variable(name: t11, value: -3)\n"
    "set_variable(name: t12, value: +3)\n"

    "set_variable(name: t21, value: 3 + 2)\n"
    "set_variable(name: t22, value: -(3 + 2))\n"
    "set_variable(name: t23, value: 20 - 4)\n"
    "set_variable(name: t24, value: 3 * 2)\n"
    "set_variable(name: t25, value: 20 / 4)\n"
    "set_variable(name: t26, value: 27 % 11)\n"

    "set_variable(name: t31, value: 3 + 2 * 5)\n"
    "set_variable(name: t32, value: -7 + 15 / 3)\n"
    "set_variable(name: t33, value: +2 + 15 % 7)\n"

    "set_variable(name: t41, value: (3 + 2) * 5)\n"
    "set_variable(name: t42, value: (-7 + 15) / 3)\n"
    "set_variable(name: t43, value: (+2 + 15) % 7)\n"
;

constexpr char const * const g_program_verify_computation_floating_point =
    "set_variable(name: t01, value: 3.01)\n"

    "set_variable(name: t11, value: -3.5)\n"
    "set_variable(name: t12, value: +3.2)\n"

    "set_variable(name: t21ff, value: 3.01 + 2.45)\n"
    "set_variable(name: t21if, value: 3 + 2.54)\n"
    "set_variable(name: t21fi, value: 3.01 + 2)\n"
    "set_variable(name: t22ff, value: -(3.5 + 2.5))\n"
    "set_variable(name: t22if, value: -(3 + 2.11))\n"
    "set_variable(name: t22fi, value: -(3.07 + 2))\n"
    "set_variable(name: t23ff, value: 20.07 - 4.13)\n"
    "set_variable(name: t23if, value: 20 - 4.78)\n"
    "set_variable(name: t23fi, value: 20.91 - 4)\n"
    "set_variable(name: t24ff, value: 3.41 * 2.14)\n"
    "set_variable(name: t24if, value: 3 * 2.67)\n"
    "set_variable(name: t24fi, value: 3.32 * 2)\n"
    "set_variable(name: t25ff, value: 20.83 / 4.07)\n"
    "set_variable(name: t25if, value: 20 / 4.4)\n"
    "set_variable(name: t25fi, value: 20.93 / 4)\n"
    "set_variable(name: t26ff, value: 27.27 % 11.11)\n"
    "set_variable(name: t26if, value: 27 % 11.88)\n"
    "set_variable(name: t26fi, value: 27.72 % 11)\n"

    "set_variable(name: t31fff, value: 3.03 + 2.2 * 5.9)\n"
    "set_variable(name: t31iff, value: 3 + 2.5 * 5.7)\n"
    "set_variable(name: t31fif, value: 3.2 + 2 * 5.3)\n"
    "set_variable(name: t31ffi, value: 3.07 + 2.28 * 5)\n"
    "set_variable(name: t31iif, value: 3 + 2 * 5.67)\n"
    "set_variable(name: t31ifi, value: 3 + 2.56 * 5)\n"
    "set_variable(name: t31fii, value: 3.33 + 2 * 5)\n"
    "set_variable(name: t32fff, value: -7.11 + 15.7 / 3.06)\n"
    "set_variable(name: t32iff, value: -7 + 15.25 / 3.31)\n"
    "set_variable(name: t32fif, value: -7.78 + 15 / 3.77)\n"
    "set_variable(name: t32ffi, value: -7.09 + 15.34 / 3)\n"
    "set_variable(name: t32iif, value: -7 + 15 / 3.30)\n"
    "set_variable(name: t32ifi, value: -7 + 15.09 / 3)\n"
    "set_variable(name: t32fii, value: -7.94 + 15 / 3)\n"
    "set_variable(name: t33fff, value: +2.21 + 15.16 % 7.8)\n"
    "set_variable(name: t33iff, value: +2 + 15.12 % 7.93)\n"
    "set_variable(name: t33fif, value: +2.58 + 15 % 7.63)\n"
    "set_variable(name: t33ffi, value: +2.12 + 15.09 % 7)\n"
    "set_variable(name: t33iif, value: +2 + 15 % 7.19)\n"
    "set_variable(name: t33ifi, value: +2 + 15.18 % 7)\n"
    "set_variable(name: t33fii, value: +2.17 + 15 % 7)\n"

    "set_variable(name: t41fff, value: (3.45 + 2.06) * 5.55)\n"
    "set_variable(name: t41iff, value: (3 + 2.17) * 5.07)\n"
    "set_variable(name: t41fif, value: (3.37 + 2) * 5.12)\n"
    "set_variable(name: t41ffi, value: (3.45 + 2.67) * 5)\n"
    "set_variable(name: t41iif, value: (3 + 2) * 5.3)\n"
    "set_variable(name: t41ifi, value: (3 + 2.9) * 5)\n"
    "set_variable(name: t41fii, value: (3.4 + 2) * 5)\n"
    "set_variable(name: t42fff, value: (-7.4 + 15.15) / 3.93)\n"
    "set_variable(name: t42iff, value: (-7 + 15.21) / 3.43)\n"
    "set_variable(name: t42fif, value: (-7.72 + 15) / 3.31)\n"
    "set_variable(name: t42ffi, value: (-7.43 + 15.89) / 3)\n"
    "set_variable(name: t42iif, value: (-7 + 15) / 3.4)\n"
    "set_variable(name: t42ifi, value: (-7 + 15.09) / 3)\n"
    "set_variable(name: t42fii, value: (-7.73 + 15) / 3)\n"
    "set_variable(name: t43fff, value: (+2.25 + 15.36) % 7.47)\n"
    "set_variable(name: t43iff, value: (+2 + 15.16) % 7.38)\n"
    "set_variable(name: t43fif, value: (+2.51 + 15) % 7.59)\n"
    "set_variable(name: t43ffi, value: (+2.4 + 15.3) % 7)\n"
    "set_variable(name: t43iif, value: (+2 + 15) % 7.0)\n"
    "set_variable(name: t43ifi, value: (+2 + 15.8) % 7)\n"
    "set_variable(name: t43fii, value: (+2.07 + 15) % 7)\n"
;

constexpr char const * const g_program_verify_computation_timestamp =
    "set_variable(name: t01, value: @123 + 5)\n"
    "set_variable(name: t02, value: 33 + @123)\n"
    "set_variable(name: t03, value: @123 - 5)\n"
    "set_variable(name: t04, value: 33 - @123)\n"

    "set_variable(name: t11, value: @123 + 5.09)\n"
    "set_variable(name: t12, value: 33.501923821 + @123)\n"
    "set_variable(name: t13, value: @123 - 5.001)\n"
    "set_variable(name: t14, value: 333.98201992 - @123)\n"

    "set_variable(name: t21, value: -@123)\n"
    "set_variable(name: t22, value: +@123)\n"
;

constexpr char const * const g_program_verify_computation_address =
    "set_variable(name: t01, value: <127.0.0.1> + 256)\n"
    "set_variable(name: t02, value: 256 + <192.168.3.57>)\n"
    "set_variable(name: t03, value: <172.131.4.1> - 256)\n"

    "set_variable(name: t11, value: <10.5.34.255> - <10.5.33.0>)\n"
;

constexpr char const * const g_program_verify_computation_concatenation =
    "set_variable(name: t01, value: ident + ifier)\n"

    "set_variable(name: t11, value: 'single' + ' ' + 'string')\n"
    "set_variable(name: t12, value: 'single' + \" \" + 'string')\n"
    "set_variable(name: t13, value: 'single' + ' ' + \"string\")\n"
    "set_variable(name: t14, value: \"double\" + \" \" + \"string\")\n"

    "set_variable(name: t21, value: +identify)\n"
    "set_variable(name: t22, value: +'single string')\n"
    "set_variable(name: t23, value: +\"double string\")\n"

    "set_variable(name: t31, value: 'single' + 36)\n"
    "set_variable(name: t32, value: 258 + 'single')\n"
    "set_variable(name: t33, value: \"string\" + 102)\n"
    "set_variable(name: t34, value: 5005 + \"double\")\n"
;

constexpr char const * const g_program_verify_computation_string_repeat =
    "set_variable(name: t01, value: 'abc' * 3)\n"
    "set_variable(name: t02, value: \"xyz\" * 5)\n"
    "set_variable(name: t03, value: \"zero\" * 0)\n"
    "set_variable(name: t04, value: \"one\" * 1)\n"
;

constexpr char const * const g_program_accept_one_message =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n" // first wait reacts on connect(), second wait receives the REGISTER message
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: 1 }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "send_message(command: READY, sent_server: reporter_test_extension, sent_service: test_processor, server: reporter_test, service: accept_one_message, parameters: { status: alive })\n"
    "wait(timeout: 10.0, mode: drain)\n"
    "disconnect()\n"
    "exit()\n"
;

constexpr char const * const g_program_receive_unwanted_message =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n" // first wait reacts on connect(), second wait receives the REGISTER message
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: 1 }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "send_message(command: READY)\n"
    //"wait(timeout: 10.0, mode: drain)\n"
    "exit(timeout: 2.5)\n"
;

constexpr char const * const g_program_send_and_receive_complete_messages =
    "run()\n"
    "set_variable(name: got_register, value: 0)\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n" // first wait reacts on connect(), second wait receives the REGISTER message
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "has_message(command: REGISTER)\n"
    "if(false: not_register)\n"
    "set_variable(name: got_register, value: 1)\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: 1 })\n"
    "send_message(command: READY, sent_server: reporter_test_extension, sent_service: test_processor, server: reporter_test, service: accept_one_message, parameters: { status: alive })\n"
    "sleep(seconds: 1)\n"
    "send_message(command: HELP, sent_server: reporter_test_extension, sent_service: test_processor, server: reporter_test, service: accept_one_message)\n"
    "goto(label: wait_message)\n"
    "label(name: not_register)\n"
    "has_message(command: COMMANDS)\n"
    "if(false: not_commands)\n"
    "verify_message(command: COMMANDS, sent_server: reporter_test, sent_service: commands_message, server: reporter_test_extension, service: test_processor, required_parameters: { list: \"HELP,READY,STOP\" })\n"
    "if(variable: got_register, false: register_missing)\n"
    "unset_variable(name: got_register)\n"
    "send_message(command: STOP)\n"
    "wait(timeout: 10.0, mode: drain)\n"
    "exit(timeout: 2.5)\n"
    "label(name: not_commands)\n"
    "exit(error_message: \"expected message REGISTER or COMMANDS.\")\n"
    "label(name: register_missing)\n"
    "exit(error_message: \"message REGISTER not received before COMMANDS message.\")\n"
;

constexpr char const * const g_program_undefined_variable =
    "has_type(name: undefined_variable, type: void)\n"
    "if(unordered: it_worked)\n"
    "exit(error_message: \"undefined variable not properly detected.\")\n"
    "label(name: it_worked)\n"
    "exit()\n"
;

constexpr char const * const g_program_integer_variable =
    "set_variable(name: my_int, value: 33)\n"
    "has_type(name: my_int, type: string)\n"
    "if(false: not_string)\n"
    "exit(error_message: \"integer variable detected as a string.\")\n"
    "label(name: not_string)\n"
    "has_type(name: my_int, type: integer)\n"
    "if(true: is_integer)\n"
    "exit(error_message: \"integer variable not properly detected as such.\")\n"
    "label(name: is_integer)\n"
    "exit()\n"
;

constexpr char const * const g_program_string_variable =
    "set_variable(name: my_str, value: \"3.3\")\n"
    "has_type(name: my_str, type: floating_point)\n"
    "if(false: not_floating_point)\n"
    "exit(error_message: \"string variable detected as a floating_point.\")\n"
    "label(name: not_floating_point)\n"
    "has_type(name: my_str, type: string)\n"
    "if(true: is_string)\n"
    "exit(error_message: \"string variable not properly detected as such.\")\n"
    "label(name: is_string)\n"
    "exit()\n"
;

constexpr char const * const g_program_if_variable =
    "if(variable: not_defined, unordered: not_defined_worked)\n"
    "exit(error_message: \"if(variable: <undefined>) failed test.\")\n"
    "label(name: not_defined_worked)\n"

    // >, >=, !=, true, ordered
    "set_variable(name: my_var, value: 5)\n"
    "if(variable: my_var, greater: positive_greater_int_worked)\n"
    "exit(error_message: \"if(variable: <positive integer> + greater) failed test.\")\n"
    "label(name: positive_greater_int_worked)\n"
    "if(variable: my_var, greater_or_equal: positive_greater_or_equal_int_worked)\n"
    "exit(error_message: \"if(variable: <positive integer> + greater_or_equal) failed test.\")\n"
    "label(name: positive_greater_or_equal_int_worked)\n"
    "if(variable: my_var, not_equal: positive_not_equal_int_worked)\n"
    "exit(error_message: \"if(variable: <positive integer> + not_equal) failed test.\")\n"
    "label(name: positive_not_equal_int_worked)\n"
    "if(variable: my_var, true: positive_true_int_worked)\n"
    "exit(error_message: \"if(variable: <positive integer> + true) failed test.\")\n"
    "label(name: positive_true_int_worked)\n"
    "if(variable: my_var, ordered: positive_ordered_int_worked)\n"
    "exit(error_message: \"if(variable: <positive integer> + ordered) failed test.\")\n"
    "label(name: positive_ordered_int_worked)\n"

    // <, <=, !=, true, ordered
    "set_variable(name: my_var, value: -5)\n"
    "if(variable: my_var, less: negative_less_int_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + less) failed test.\")\n"
    "label(name: negative_less_int_worked)\n"
    "if(variable: my_var, less_or_equal: negative_less_or_equal_int_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + less_or_equal) failed test.\")\n"
    "label(name: negative_less_or_equal_int_worked)\n"
    "if(variable: my_var, not_equal: negative_not_equal_int_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + not_equal) failed test.\")\n"
    "label(name: negative_not_equal_int_worked)\n"
    "if(variable: my_var, true: negative_true_int_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + true) failed test.\")\n"
    "label(name: negative_true_int_worked)\n"
    "if(variable: my_var, ordered: negative_ordered_int_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + ordered) failed test.\")\n"
    "label(name: negative_ordered_int_worked)\n"

    // ==, false, ordered
    "set_variable(name: my_var, value: 0)\n"
    "if(variable: my_var, equal: zero_equal_int_worked)\n"
    "exit(error_message: \"if(variable: <zero integer> + equal) failed test.\")\n"
    "label(name: zero_equal_int_worked)\n"
    "if(variable: my_var, less_or_equal: zero_less_or_equal_int_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + less_or_equal) failed test.\")\n"
    "label(name: zero_less_or_equal_int_worked)\n"
    "if(variable: my_var, greater_or_equal: zero_greater_or_equal_int_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + greater_or_equal) failed test.\")\n"
    "label(name: zero_greater_or_equal_int_worked)\n"
    "if(variable: my_var, false: zero_false_int_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + false) failed test.\")\n"
    "label(name: zero_false_int_worked)\n"
    "if(variable: my_var, ordered: zero_ordered_int_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + ordered) failed test.\")\n"
    "label(name: zero_ordered_int_worked)\n"

    // >, >=, !=, true, ordered
    "set_variable(name: my_var, value: 7.3)\n"
    "if(variable: my_var, greater: positive_greater_flt_worked)\n"
    "exit(error_message: \"if(variable: <positive floating point> + greater) failed test.\")\n"
    "label(name: positive_greater_flt_worked)\n"
    "if(variable: my_var, greater_or_equal: positive_greater_or_equal_flt_worked)\n"
    "exit(error_message: \"if(variable: <positive floating point> + greater_or_equal) failed test.\")\n"
    "label(name: positive_greater_or_equal_flt_worked)\n"
    "if(variable: my_var, not_equal: positive_not_equal_flt_worked)\n"
    "exit(error_message: \"if(variable: <positive floating point> + not_equal) failed test.\")\n"
    "label(name: positive_not_equal_flt_worked)\n"
    "if(variable: my_var, true: positive_true_flt_worked)\n"
    "exit(error_message: \"if(variable: <positive floating point> + true) failed test.\")\n"
    "label(name: positive_true_flt_worked)\n"
    "if(variable: my_var, ordered: positive_ordered_flt_worked)\n"
    "exit(error_message: \"if(variable: <positive floating point> + ordered) failed test.\")\n"
    "label(name: positive_ordered_flt_worked)\n"

    // <, <=, !=, true, ordered
    "set_variable(name: my_var, value: -7.3)\n"
    "if(variable: my_var, less: negative_less_flt_worked)\n"
    "exit(error_message: \"if(variable: <negative floating point> + less) failed test.\")\n"
    "label(name: negative_less_flt_worked)\n"
    "if(variable: my_var, less_or_equal: negative_less_or_equal_flt_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + less_or_equal) failed test.\")\n"
    "label(name: negative_less_or_equal_flt_worked)\n"
    "if(variable: my_var, not_equal: negative_not_equal_flt_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + not_equal) failed test.\")\n"
    "label(name: negative_not_equal_flt_worked)\n"
    "if(variable: my_var, true: negative_true_flt_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + true) failed test.\")\n"
    "label(name: negative_true_flt_worked)\n"
    "if(variable: my_var, ordered: negative_ordered_flt_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + ordered) failed test.\")\n"
    "label(name: negative_ordered_flt_worked)\n"

    // ==, false, ordered
    "set_variable(name: my_var, value: 0.0)\n"
    "if(variable: my_var, equal: zero_equal_flt_worked)\n"
    "exit(error_message: \"if(variable: <zero floating point> + equal) failed test.\")\n"
    "label(name: zero_equal_flt_worked)\n"
    "if(variable: my_var, less_or_equal: zero_less_or_equal_flt_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + less_or_equal) failed test.\")\n"
    "label(name: zero_less_or_equal_flt_worked)\n"
    "if(variable: my_var, greater_or_equal: zero_greater_or_equal_flt_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + greater_or_equal) failed test.\")\n"
    "label(name: zero_greater_or_equal_flt_worked)\n"
    "if(variable: my_var, false: zero_false_flt_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + false) failed test.\")\n"
    "label(name: zero_false_flt_worked)\n"
    "if(variable: my_var, ordered: zero_ordered_flt_worked)\n"
    "exit(error_message: \"if(variable: <negative integer> + ordered) failed test.\")\n"
    "label(name: zero_ordered_flt_worked)\n"

    // unordered
    "set_variable(name: my_var, value: NaN)\n"
    "if(variable: my_var, unordered: unordered_flt_worked)\n"
    "exit(error_message: \"if(variable: <unordered floating point>) failed test.\")\n"
    "label(name: unordered_flt_worked)\n"

    "exit()\n"
;

constexpr char const * const g_program_error_message =
    "exit(error_message: \"testing exit with an error\")\n"
;

constexpr char const * const g_program_no_condition =
    "if(true: exit)\n"
    "label(name: exit)\n"
;

constexpr char const * const g_program_two_listen =
    "listen(address: <127.0.0.1:20002>)\n"
    "listen(address: <127.0.0.1:20003>)\n"
;

constexpr char const * const g_program_label_bad_type =
    "label(name: 123)\n"
;

constexpr char const * const g_program_exit_bad_type =
    "exit(error_message: 12.3)\n"
;

constexpr char const * const g_program_unsupported_addition_address_address =
    "set_variable(name: bad, value: <127.0.0.1:80> + <127.0.1.5:81>)\n"
;

constexpr char const * const g_program_unsupported_addition_address_string =
    "set_variable(name: bad, value: <127.0.0.1:80> + '127.0.1.5:81')\n"
;

constexpr char const * const g_program_unsupported_addition_string_address =
    "set_variable(name: bad, value: '127.0.0.1:80' + <127.0.1.5:81>)\n"
;

constexpr char const * const g_program_unsupported_addition_address_identifier =
    "set_variable(name: bad, value: <127.0.0.1:80> + alpha)\n"
;

constexpr char const * const g_program_unsupported_addition_identifier_address =
    "set_variable(name: bad, value: beta + <127.0.1.5:81>)\n"
;

constexpr char const * const g_program_unsupported_addition_identifier_string =
    "set_variable(name: bad, value: this + '127.0.1.5:81')\n"
;

constexpr char const * const g_program_unsupported_addition_string_identifier =
    "set_variable(name: bad, value: '127.0.0.1:80' + that)\n"
;

constexpr char const * const g_program_unsupported_subtraction_address_string =
    "set_variable(name: bad, value: <127.0.0.1:80> - '127.0.1.5:81')\n"
;

constexpr char const * const g_program_unsupported_subtraction_string_address =
    "set_variable(name: bad, value: '127.0.0.1:80' - <127.0.1.5:81>)\n"
;

constexpr char const * const g_program_unsupported_subtraction_address_identifier =
    "set_variable(name: bad, value: <127.0.0.1:80> - alpha)\n"
;

constexpr char const * const g_program_unsupported_subtraction_identifier_address =
    "set_variable(name: bad, value: beta - <127.0.1.5:81>)\n"
;

constexpr char const * const g_program_unsupported_subtraction_identifier_string =
    "set_variable(name: bad, value: this - '127.0.1.5:81')\n"
;

constexpr char const * const g_program_unsupported_subtraction_string_identifier =
    "set_variable(name: bad, value: '127.0.0.1:80' - that)\n"
;

constexpr char const * const g_program_unsupported_multiplication_address_address =
    "set_variable(name: bad, value: <127.0.0.1:80> * <192.168.2.2:443>)\n"
;

constexpr char const * const g_program_unsupported_multiplication_address_string =
    "set_variable(name: bad, value: <127.0.0.1:80> * 'invalid')\n"
;

constexpr char const * const g_program_unsupported_multiplication_string_address =
    "set_variable(name: bad, value: 'invalid' * <127.0.0.1:80>)\n"
;

constexpr char const * const g_program_unsupported_multiplication_address_identifier =
    "set_variable(name: bad, value: <127.0.0.1:80> * invalid)\n"
;

constexpr char const * const g_program_unsupported_multiplication_identifier_address =
    "set_variable(name: bad, value: invalid * <127.0.0.1:80>)\n"
;

constexpr char const * const g_program_unsupported_multiplication_identifier_string =
    "set_variable(name: bad, value: 'invalid' * invalid)\n"
;

constexpr char const * const g_program_unsupported_multiplication_string_identifier =
    "set_variable(name: bad, value: invalid * \"invalid\")\n"
;

constexpr char const * const g_program_unsupported_multiplication_string_string =
    "set_variable(name: bad, value: 'invalid' * \"invalid\")\n"
;

constexpr char const * const g_program_unsupported_multiplication_identifier_identifier =
    "set_variable(name: bad, value: invalid * not_valid)\n"
;

constexpr char const * const g_program_unsupported_division_address_address =
    "set_variable(name: bad, value: <127.0.0.1:80> / <192.168.2.2:443>)\n"
;

constexpr char const * const g_program_unsupported_division_address_string =
    "set_variable(name: bad, value: <127.0.0.1:80> / 'invalid')\n"
;

constexpr char const * const g_program_unsupported_division_string_address =
    "set_variable(name: bad, value: 'invalid' / <127.0.0.1:80>)\n"
;

constexpr char const * const g_program_unsupported_division_address_identifier =
    "set_variable(name: bad, value: <127.0.0.1:80> / invalid)\n"
;

constexpr char const * const g_program_unsupported_division_identifier_address =
    "set_variable(name: bad, value: invalid / <127.0.0.1:80>)\n"
;

constexpr char const * const g_program_unsupported_division_identifier_string =
    "set_variable(name: bad, value: 'invalid' / invalid)\n"
;

constexpr char const * const g_program_unsupported_division_string_identifier =
    "set_variable(name: bad, value: invalid / \"invalid\")\n"
;

constexpr char const * const g_program_unsupported_division_string_string =
    "set_variable(name: bad, value: 'invalid' / \"invalid\")\n"
;

constexpr char const * const g_program_unsupported_division_identifier_identifier =
    "set_variable(name: bad, value: invalid / not_valid)\n"
;

constexpr char const * const g_program_unsupported_modulo_address_address =
    "set_variable(name: bad, value: <127.0.0.1:80> % <192.168.2.2:443>)\n"
;

constexpr char const * const g_program_unsupported_modulo_address_string =
    "set_variable(name: bad, value: <127.0.0.1:80> % 'invalid')\n"
;

constexpr char const * const g_program_unsupported_modulo_string_address =
    "set_variable(name: bad, value: 'invalid' % <127.0.0.1:80>)\n"
;

constexpr char const * const g_program_unsupported_modulo_address_identifier =
    "set_variable(name: bad, value: <127.0.0.1:80> % invalid)\n"
;

constexpr char const * const g_program_unsupported_modulo_identifier_address =
    "set_variable(name: bad, value: invalid % <127.0.0.1:80>)\n"
;

constexpr char const * const g_program_unsupported_modulo_identifier_string =
    "set_variable(name: bad, value: 'invalid' % invalid)\n"
;

constexpr char const * const g_program_unsupported_modulo_string_identifier =
    "set_variable(name: bad, value: invalid % \"invalid\")\n"
;

constexpr char const * const g_program_unsupported_modulo_string_string =
    "set_variable(name: bad, value: 'invalid' % \"invalid\")\n"
;

constexpr char const * const g_program_unsupported_modulo_identifier_identifier =
    "set_variable(name: bad, value: invalid % not_valid)\n"
;

constexpr char const * const g_program_unsupported_negation_single_string =
    "set_variable(name: bad, value: -'string')\n"
;

constexpr char const * const g_program_unsupported_negation_double_string =
    "set_variable(name: bad, value: -\"string\")\n"
;

constexpr char const * const g_program_unsupported_negation_address =
    "set_variable(name: bad, value: -<127.0.0.1:80>)\n"
;

constexpr char const * const g_program_unsupported_negation_repeat =
    "set_variable(name: bad, value: 'string' * -5)\n"
;

constexpr char const * const g_program_unsupported_large_repeat =
    "set_variable(name: bad, value: 'string' * 1001)\n"
;

constexpr char const * const g_program_bad_exit =
    "exit(error_message: \"bad error occurred!\", timeout: 3.001)\n"
;

constexpr char const * const g_program_bad_exit_timeout =
    "exit(timeout: 'bad')\n"
;

constexpr char const * const g_program_send_message_without_connection =
    "send_message(command: WITHOUT_CONNECTION)\n"
;

constexpr char const * const g_program_if_invalid_type =
    "set_variable(name: my_str, value: \"bad\")\n"
    "if(variable: my_str, unordered: unused)\n"
    "exit(error_message: \"if() did not fail.\")\n"
    "label(name: unused)\n"
    "exit(error_message: \"if() branched unexpectendly.\")\n"
;

constexpr char const * const g_program_wait_outside_thread =
    "wait(timeout: 10)\n"
;

constexpr char const * const g_program_wait_invalid_mode =
    "run()\n"
    "wait(timeout: 10, mode: not_this_one)\n"
;

constexpr char const * const g_program_wait_no_connections =
    "run()\n"
    "wait(timeout: 10, mode: wait)\n"
;

constexpr char const * const g_program_verify_message_fail_sent_server =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, sent_server: not_this_one)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_sent_service =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, sent_service: not_this_one)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_server =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, server: not_this_one)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_service =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, service: not_this_one)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_command =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: NOT_THIS_ONE)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_forbidden =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, forbidden_parameters: { version })\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_required =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { not_this_one: 123 })\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_required_int_value =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { version: 200 })\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_required_str_value =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: not_this_one })\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_required_flt_value =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { version: 1.0 })\n"
    "exit()\n"
;

constexpr char const * const g_program_last_wait =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n" // first wait reacts on connect(), second wait receives the REGISTER message
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: 1 }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "send_message(command: READY, sent_server: reporter_test_extension, sent_service: test_processor, server: reporter_test, service: accept_one_message, parameters: { status: alive })\n"
    "wait(timeout: 1.0, mode: drain)\n"
    "wait(timeout: 1.0)\n"
    "disconnect()\n"
    "exit()\n"
;

constexpr char const * const g_program_wait_for_nothing =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n" // first wait reacts on connect(), second wait receives the REGISTER message
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: 1 }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "send_message(command: READY, sent_server: reporter_test_extension, sent_service: test_processor, server: reporter_test, service: accept_one_message, parameters: { status: alive })\n"
    "wait(timeout: 1.0, mode: drain)\n"
    "wait(timeout: 1.0)\n"
    "wait(timeout: 1.0)\n" // one too many wait(), it will time out
    "exit()\n"
;



struct expected_trace_t
{
    SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t const
                        f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_BEFORE_CALL;
    char const * const  f_name = nullptr;
};


constexpr expected_trace_t const g_verify_starting_thread[] =
{
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_BEFORE_CALL,
        .f_name = "set_variable",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_AFTER_CALL,
        .f_name = "set_variable",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_BEFORE_CALL,
        .f_name = "run",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_AFTER_CALL,
        .f_name = "run",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_BEFORE_CALL,
        .f_name = "set_variable",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_AFTER_CALL,
        .f_name = "set_variable",
    },
    {}
};


class trace
{
public:
    trace(expected_trace_t const * expected_trace)
        : f_expected_trace(expected_trace)
    {
    }

    trace(trace const &) = delete;

    ~trace()
    {
        // make sure we reached the end of the list
        //
        CATCH_REQUIRE(f_expected_trace[f_pos].f_name == nullptr);
    }

    trace operator = (trace const &) = delete;

    void callback(SNAP_CATCH2_NAMESPACE::reporter::state & s, SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t reason)
    {
        // here we can be in the thread so DO NOT USE CATCH_... macros
        //
        if(f_expected_trace[f_pos].f_name == nullptr)
        {
            throw std::runtime_error(
                  "got more calls ("
                + std::to_string(f_pos + 1)
                + ") to tracer than expected.");
        }

        if(f_expected_trace[f_pos].f_reason != reason)
        {
            throw std::runtime_error(
                  "unexpected reason at position "
                + std::to_string(f_pos)
                + " (got "
                + std::to_string(static_cast<int>(reason))
                + ", expected "
                + std::to_string(static_cast<int>(f_expected_trace[f_pos].f_reason))
                + ").");
        }

        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t stmt(s.get_running_statement());
        std::string const & name(stmt->get_instruction()->get_name());
//std::cerr << "--------------------- at pos " << f_pos << " found reason " << static_cast<int>(reason) << " + name " << name << "\n";
        if(f_expected_trace[f_pos].f_name != name)
        {
            throw std::runtime_error(
                  "unexpected instruction at position "
                + std::to_string(f_pos)
                + " (got "
                + name
                + ", expected "
                + f_expected_trace[f_pos].f_name
                + ").");
        }

        ++f_pos;
    }

private:
    int                         f_pos = 0;
    expected_trace_t const *    f_expected_trace = nullptr;
};


class messenger_responder // an equivalent to a client
    : public ed::tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<messenger_responder> pointer_t;

    enum class sequence_t
    {
        SEQUENCE_ONE_MESSAGE,
        SEQUENCE_UNWANTED_MESSAGE,
        SEQUENCE_READY_HELP_MESSAGE,
    };

    messenger_responder(
              addr::addr const & a
            , ed::mode_t mode
            , sequence_t sequence)
        : tcp_client_permanent_message_connection(
              a
            , mode
            , ed::DEFAULT_PAUSE_BEFORE_RECONNECTING
            , true
            , "responder")  // service name
        , f_sequence(sequence)
    {
        set_name("messenger_responder");    // connection name
    }

    virtual void process_connected() override
    {
        // always register at the time we connect
        //
        tcp_client_permanent_message_connection::process_connected();
        register_service();
    }

    void process_message(ed::message & msg)
    {
        ++f_step;
        std::cout
            << "--- \"client\" message ("
            << f_step
            << "): "
            << msg
            << std::endl;

        bool disconnect_all(false);

        if(f_step == 1)
        {
            if(msg.get_command() != "READY")
            {
                throw std::runtime_error(
                      "first message expected to be READY, got "
                    + msg.get_command()
                    + " instead.");
            }
        }

        switch(f_sequence)
        {
        case sequence_t::SEQUENCE_ONE_MESSAGE:
            disconnect_all = true;
            break;

        case sequence_t::SEQUENCE_UNWANTED_MESSAGE:
            {
                ed::message unwanted;
                unwanted.reply_to(msg);
                unwanted.set_command("UNWANTED");
                unwanted.add_parameter("serial", 7209);
                if(!send_message(unwanted, false))
                {
                    throw std::runtime_error("could not send UNWANTED message");
                }
                disconnect_all = true;
            }
            break;

        case sequence_t::SEQUENCE_READY_HELP_MESSAGE:
            switch(f_step)
            {
            case 1:
                // done in this case
                break;

            case 2:
                if(msg.get_command() != "HELP")
                {
                    throw std::runtime_error(
                          "second message expected to be HELP, got "
                        + msg.get_command()
                        + " instead.");
                }

                {
                    ed::message commands;
                    commands.reply_to(msg);
                    commands.set_sent_from_server("reporter_test");
                    commands.set_sent_from_service("commands_message");
                    commands.set_command("COMMANDS");
                    commands.add_parameter("list", "HELP,READY,STOP");
//std::cerr << "--- respond with COMMANDS\n";
                    if(!send_message(commands, false))
                    {
                        throw std::runtime_error("could not send COMMANDS message");
                    }
                }
                break;

            case 3:
                if(msg.get_command() != "STOP")
                {
                    throw std::runtime_error(
                          "third message expected to be STOP, got "
                        + msg.get_command()
                        + " instead.");
                }

                disconnect_all = true;
                break;

            default:
                throw std::runtime_error("reached step 4 of SEQUENCE_READY_HELP_MESSAGE?");

            }
            break;

        }

        if(disconnect_all)
        {
            remove_from_communicator();

            ed::connection::pointer_t timer_ptr(f_timer.lock());
            if(timer_ptr != nullptr)
            {
                timer_ptr->remove_from_communicator();
            }
        }
    }

    void set_timer(ed::connection::pointer_t done_timer)
    {
        f_timer = done_timer;
    }

private:
    // the sequence & step define the next action
    //
    sequence_t  f_sequence = sequence_t::SEQUENCE_ONE_MESSAGE;
    int         f_step = 0;
    ed::connection::weak_pointer_t
                f_timer = ed::connection::weak_pointer_t();
};


class messenger_timer
    : public ed::timer
{
public:
    typedef std::shared_ptr<messenger_timer>        pointer_t;

    messenger_timer(messenger_responder::pointer_t m)
        : timer(10'000'000)
        , f_messenger(m)
    {
        set_name("messenger_timer");
    }

    void process_timeout()
    {
        remove_from_communicator();
        f_messenger->remove_from_communicator();
        f_timed_out = true;
    }

    bool timed_out_prima() const
    {
        return f_timed_out;
    }

private:
    messenger_responder::pointer_t      f_messenger = messenger_responder::pointer_t();
    bool                                f_timed_out = false;
};






} // no name namespace



CATCH_TEST_CASE("reporter_executor", "[executor][reporter]")
{
    CATCH_START_SECTION("verify sleep in a function")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_sleep_func", g_program_sleep_func));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 5);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        snapdev::timespec_ex const start(snapdev::now());
        e->start();
        e->run();
        snapdev::timespec_ex end(snapdev::now());
        end -= start;
        CATCH_REQUIRE(end.tv_sec >= 2); // we slept for 2.5 seconds, so we expect at least start + 2 seconds
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify starting the thread")
    {
        trace tracer(g_verify_starting_thread);

        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_start_thread", g_program_start_thread));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());

        // use std::bind() to avoid copies of the tracer object
        //
        s->set_trace_callback(std::bind(&trace::callback, &tracer, std::placeholders::_1, std::placeholders::_2));

        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 3);

        // before we run the script, there are no such variables
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("test"));
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("runner");
        CATCH_REQUIRE(var == nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        var = s->get_variable("test");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "test");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 33);

        var = s->get_variable("runner");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "runner");
        CATCH_REQUIRE(var->get_type() == "floating_point");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_floating_point>(var)->get_floating_point() == 6.07);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify computation (integers)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_integer", g_program_verify_computation_integer));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 15);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;

        var = s->get_variable("t01");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t01");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 3);

        var = s->get_variable("t11");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t11");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == -3);

        var = s->get_variable("t12");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t12");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == +3);

        var = s->get_variable("t21");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t21");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 3 + 2);

        var = s->get_variable("t22");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t22");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == -(3 + 2));

        var = s->get_variable("t23");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t23");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 20 - 4);

        var = s->get_variable("t24");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t24");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 3 * 2);

        var = s->get_variable("t25");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t25");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 20 / 4);

        var = s->get_variable("t26");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t26");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 27 % 11);

        var = s->get_variable("t31");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t31");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 3 + 2 * 5);

        var = s->get_variable("t32");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t32");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == -7 + 15 / 3);

        var = s->get_variable("t33");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t33");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == +2 + 15 % 7);

        var = s->get_variable("t41");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t41");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == (3 + 2) * 5);

        var = s->get_variable("t42");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t42");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == (-7 + 15) / 3);

        var = s->get_variable("t43");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t43");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == (+2 + 15) % 7);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify computation (floating points)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_floating_point", g_program_verify_computation_floating_point));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 63);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        struct verify_t
        {
            char const *    f_name = nullptr;
            double          f_value = 0.0;
        };

        verify_t verify[] =
        {
            { "t01", 3.01 },
            { "t11", -3.5 },
            { "t12", +3.2 },
            { "t21ff", 3.01 + 2.45 },
            { "t21if", 3 + 2.54 },
            { "t21fi", 3.01 + 2 },
            { "t22ff", -(3.5 + 2.5) },
            { "t22if", -(3 + 2.11) },
            { "t22fi", -(3.07 + 2) },
            { "t23ff", 20.07 - 4.13 },
            { "t23if", 20 - 4.78 },
            { "t23fi", 20.91 - 4 },
            { "t24ff", 3.41 * 2.14 },
            { "t24if", 3 * 2.67 },
            { "t24fi", 3.32 * 2 },
            { "t25ff", 20.83 / 4.07 },
            { "t25if", 20 / 4.4 },
            { "t25fi", 20.93 / 4 },
            { "t26ff", fmod(27.27, 11.11) },
            { "t26if", fmod(27, 11.88) },
            { "t26fi", fmod(27.72, 11) },
            { "t31fff", 3.03 + 2.2 * 5.9 },
            { "t31iff", 3 + 2.5 * 5.7 },
            { "t31fif", 3.2 + 2 * 5.3 },
            { "t31ffi", 3.07 + 2.28 * 5 },
            { "t31iif", 3 + 2 * 5.67 },
            { "t31ifi", 3 + 2.56 * 5 },
            { "t31fii", 3.33 + 2 * 5 },
            { "t32fff", -7.11 + 15.7 / 3.06 },
            { "t32iff", -7 + 15.25 / 3.31 },
            { "t32fif", -7.78 + 15 / 3.77 },
            { "t32ffi", -7.09 + 15.34 / 3 },
            { "t32iif", -7 + 15 / 3.30 },
            { "t32ifi", -7 + 15.09 / 3 },
            { "t32fii", -7.94 + 15 / 3 },
            { "t33fff", +2.21 + fmod(15.16, 7.8) },
            { "t33iff", +2 + fmod(15.12, 7.93) },
            { "t33fif", +2.58 + fmod(15, 7.63) },
            { "t33ffi", +2.12 + fmod(15.09, 7) },
            { "t33iif", +2 + fmod(15, 7.19) },
            { "t33ifi", +2 + fmod(15.18, 7) },
            { "t33fii", +2.17 + fmod(15, 7) },
            { "t41fff", (3.45 + 2.06) * 5.55 },
            { "t41iff", (3 + 2.17) * 5.07 },
            { "t41fif", (3.37 + 2) * 5.12 },
            { "t41ffi", (3.45 + 2.67) * 5 },
            { "t41iif", (3 + 2) * 5.3 },
            { "t41ifi", (3 + 2.9) * 5 },
            { "t41fii", (3.4 + 2) * 5 },
            { "t42fff", (-7.4 + 15.15) / 3.93 },
            { "t42iff", (-7 + 15.21) / 3.43 },
            { "t42fif", (-7.72 + 15) / 3.31 },
            { "t42ffi", (-7.43 + 15.89) / 3 },
            { "t42iif", (-7 + 15) / 3.4 },
            { "t42ifi", (-7 + 15.09) / 3 },
            { "t42fii", (-7.73 + 15) / 3 },
            { "t43fff", fmod((+2.25 + 15.36), 7.47) },
            { "t43iff", fmod((+2 + 15.16), 7.38) },
            { "t43fif", fmod((+2.51 + 15), 7.59) },
            { "t43ffi", fmod((+2.4 + 15.3), 7) },
            { "t43iif", fmod((+2 + 15), 7.0) },
            { "t43ifi", fmod((+2 + 15.8), 7) },
            { "t43fii", fmod((+2.07 + 15), 7) },
        };

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        for(auto const & v : verify)
        {
//std::cerr << "--- testing \"" << v.f_name << "\".\n";
            var = s->get_variable(v.f_name);
            CATCH_REQUIRE(var != nullptr);
            CATCH_REQUIRE(var->get_name() == v.f_name);
            CATCH_REQUIRE(var->get_type() == "floating_point");
            CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_floating_point>(var)->get_floating_point() == v.f_value);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify computation (timestamp)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_timestamp", g_program_verify_computation_timestamp));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 10);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        struct verify_t
        {
            char const *            f_name = nullptr;
            snapdev::timespec_ex    f_value = snapdev::timespec_ex();
        };

        verify_t verify[] =
        {
            { "t01", { 123 + 5, 0 } },
            { "t02", { 33 + 123, 0 } },
            { "t03", { 123 - 5, 0 } },
            { "t04", { 33 - 123, 0 } },
            { "t11", { 123 + 5, 89'999'999 } },
            { "t12", { 33 + 123, 501'923'820 } },
            { "t13", { 123 - 6, 999'000'000 } },
            { "t14", { 333 - 123, 982'019'920 } },
            { "t21", { -123, 0 } },
            { "t22", { 123, 0 } },
        };

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        for(auto const & v : verify)
        {
//std::cerr << "--- testing \"" << v.f_name << "\".\n";
            var = s->get_variable(v.f_name);
            CATCH_REQUIRE(var != nullptr);
            CATCH_REQUIRE(var->get_name() == v.f_name);
            CATCH_REQUIRE(var->get_type() == "timestamp");
            CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_timestamp>(var)->get_timestamp() == v.f_value);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify computation (address)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_address", g_program_verify_computation_address));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 4);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

    //"set_variable(name: t01, value: <127.0.0.1> + 256)\n"
    //"set_variable(name: t02, value: 256 + <192.168.3.57>)\n"
    //"set_variable(name: t03, value: <172.131.4.1> - 256)\n"
    //"set_variable(name: t11, value: <10.5.34.255> - <10.5.33.0>)\n"

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        addr::addr a;

        var = s->get_variable("t01");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t01");
        CATCH_REQUIRE(var->get_type() == "address");
        a = addr::string_to_addr("127.0.1.1");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_address>(var)->get_address() == a);

        var = s->get_variable("t02");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t02");
        CATCH_REQUIRE(var->get_type() == "address");
        a = addr::string_to_addr("192.168.4.57");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_address>(var)->get_address() == a);

        var = s->get_variable("t03");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t03");
        CATCH_REQUIRE(var->get_type() == "address");
        a = addr::string_to_addr("172.131.3.1");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_address>(var)->get_address() == a);

        var = s->get_variable("t11");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "t11");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 511);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify computation (concatenation)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_concatenation", g_program_verify_computation_concatenation));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 12);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        struct verify_t
        {
            char const *            f_name = nullptr;
            char const *            f_value = nullptr;
        };

        verify_t verify[] =
        {
            { "t01", "identifier" },
            { "t11", "single string" },
            { "t12", "single string" },
            { "t13", "single string" },
            { "t14", "double string" },
            { "t21", "identify" },
            { "t22", "single string" },
            { "t23", "double string" },
            { "t31", "single36" },
            { "t32", "258single" },
            { "t33", "string102" },
            { "t34", "5005double" },
        };

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        for(auto const & v : verify)
        {
//std::cerr << "--- testing \"" << v.f_name << "\".\n";
            var = s->get_variable(v.f_name);
            CATCH_REQUIRE(var != nullptr);
            CATCH_REQUIRE(var->get_name() == v.f_name);
            CATCH_REQUIRE(var->get_type() == "string");
            CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string() == v.f_value);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify computation (string repeat)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_string_repeat", g_program_verify_computation_string_repeat));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 4);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        struct verify_t
        {
            char const *            f_name = nullptr;
            char const *            f_value = nullptr;
        };

        verify_t verify[] =
        {
            { "t01", "abcabcabc" },
            { "t02", "xyzxyzxyzxyzxyz" },
            { "t03", "" },
            { "t04", "one" },
        };

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        for(auto const & v : verify)
        {
//std::cerr << "--- testing \"" << v.f_name << "\".\n";
            var = s->get_variable(v.f_name);
            CATCH_REQUIRE(var != nullptr);
            CATCH_REQUIRE(var->get_name() == v.f_name);
            CATCH_REQUIRE(var->get_type() == "string");
            CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string() == v.f_value);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_executor_message", "[executor][reporter]")
{
    CATCH_START_SECTION("send/receive one message")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_accept_one_message", g_program_accept_one_message));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 13);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        addr::addr a;
        sockaddr_in ip = {
            .sin_family = AF_INET,
            .sin_port = htons(20002),
            .sin_addr = {
                .s_addr = htonl(0x7f000001),
            },
            .sin_zero = {},
        };
        a.set_ipv4(ip);
        messenger_responder::pointer_t messenger(std::make_shared<messenger_responder>(
                  a
                , ed::mode_t::MODE_PLAIN
                , messenger_responder::sequence_t::SEQUENCE_ONE_MESSAGE));
        ed::communicator::instance()->add_connection(messenger);
        messenger_timer::pointer_t timer(std::make_shared<messenger_timer>(messenger));
        ed::communicator::instance()->add_connection(timer);
        messenger->set_timer(timer);

        e->run();

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("receive one unwanted/unexpected message")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_receive_unwanted_message", g_program_receive_unwanted_message));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 11);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        addr::addr a;
        sockaddr_in ip = {
            .sin_family = AF_INET,
            .sin_port = htons(20002),
            .sin_addr = {
                .s_addr = htonl(0x7f000001),
            },
            .sin_zero = {},
        };
        a.set_ipv4(ip);
        messenger_responder::pointer_t messenger(std::make_shared<messenger_responder>(
                  a
                , ed::mode_t::MODE_PLAIN
                , messenger_responder::sequence_t::SEQUENCE_UNWANTED_MESSAGE));
        ed::communicator::instance()->add_connection(messenger);
        e->set_thread_done_callback([messenger]()
            {
                ed::communicator::instance()->remove_connection(messenger);
            });

        e->run();

        CATCH_REQUIRE(s->get_exit_code() == 1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("send & receive complete messages")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_send_and_receive_complete_message", g_program_send_and_receive_complete_messages));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 30);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        addr::addr a;
        sockaddr_in ip = {
            .sin_family = AF_INET,
            .sin_port = htons(20002),
            .sin_addr = {
                .s_addr = htonl(0x7f000001),
            },
            .sin_zero = {},
        };
        a.set_ipv4(ip);
        messenger_responder::pointer_t messenger(std::make_shared<messenger_responder>(
                  a
                , ed::mode_t::MODE_PLAIN
                , messenger_responder::sequence_t::SEQUENCE_READY_HELP_MESSAGE));
        ed::communicator::instance()->add_connection(messenger);
        messenger_timer::pointer_t timer(std::make_shared<messenger_timer>(messenger));
        ed::communicator::instance()->add_connection(timer);
        messenger->set_timer(timer);

        e->run();

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == 0);

        // we unset that variable, make sure that worked
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("got_register"));
        CATCH_REQUIRE(var == nullptr);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify last wait (disconnect -> HUP)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_verify_last_wait", g_program_last_wait));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        addr::addr a;
        sockaddr_in ip = {
            .sin_family = AF_INET,
            .sin_port = htons(20002),
            .sin_addr = {
                .s_addr = htonl(0x7f000001),
            },
            .sin_zero = {},
        };
        a.set_ipv4(ip);
        messenger_responder::pointer_t messenger(std::make_shared<messenger_responder>(
                  a
                , ed::mode_t::MODE_PLAIN
                , messenger_responder::sequence_t::SEQUENCE_ONE_MESSAGE));
        ed::communicator::instance()->add_connection(messenger);
        messenger_timer::pointer_t timer(std::make_shared<messenger_timer>(messenger));
        ed::communicator::instance()->add_connection(timer);
        messenger->set_timer(timer);
        e->set_thread_done_callback([messenger, timer]()
            {
                ed::communicator::instance()->remove_connection(messenger);
                ed::communicator::instance()->remove_connection(timer);
            });
        e->run();
        e->stop();

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_executor_variables", "[executor][reporter][variable]")
{
    CATCH_START_SECTION("undefined variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_undefined_variable", g_program_undefined_variable));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 5);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("detect integer variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_integer_variable", g_program_integer_variable));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 10);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("detect string variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_string_variable", g_program_string_variable));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 10);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("if variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_if_variable", g_program_if_variable));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 104);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("void variable cloning")
    {
        // Note: at the moment there is no call to the clone() function
        //       inside the library, so make sure it works as expected
        //       within the test
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::variable_void>("void_var"));
        CATCH_REQUIRE(var != nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::variable_void::pointer_t v(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_void>(var));
        CATCH_REQUIRE(v != nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t clone(v->clone("clone"));
        CATCH_REQUIRE(clone != nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::variable_void::pointer_t c(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_void>(clone));
        CATCH_REQUIRE(c != nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t clone2(var->clone("clone2"));
        CATCH_REQUIRE(clone2 != nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::variable_void::pointer_t c2(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_void>(clone2));
        CATCH_REQUIRE(c2 != nullptr);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("list variable")
    {
        // Note: some of the list variable functions are not fully tested
        //       from within the app. so test more here
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t list(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::variable_list>("list_var"));
        CATCH_REQUIRE(list != nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::variable_list::pointer_t l(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_list>(list));
        CATCH_REQUIRE(l != nullptr);

        CATCH_REQUIRE(l->get_item_size() == 0);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var1(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::variable_void>("void_var"));
        CATCH_REQUIRE(var1 != nullptr);
        l->add_item(var1);
        CATCH_REQUIRE(l->get_item(0) == var1);
        CATCH_REQUIRE(l->get_item(-1) == nullptr);
        CATCH_REQUIRE(l->get_item(1) == nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var2(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>("integer_var"));
        CATCH_REQUIRE(var2 != nullptr);
        l->add_item(var2);
        CATCH_REQUIRE(l->get_item(0) == var2);      // this is a map so the order is sorted by variable name
        CATCH_REQUIRE(l->get_item(-1) == nullptr);
        CATCH_REQUIRE(l->get_item(1) == var1);
        CATCH_REQUIRE(l->get_item(2) == nullptr);
        CATCH_REQUIRE(l->get_item("void_var") == var1);
        CATCH_REQUIRE(l->get_item("integer_var") == var2);
        CATCH_REQUIRE(l->get_item("undefined_var") == nullptr);

        CATCH_REQUIRE_THROWS_MATCHES(
              l->add_item(var1)
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage("variable_list::add_item() trying to re-add item named \"void_var\"."));

        CATCH_REQUIRE_THROWS_MATCHES(
              l->add_item(var2)
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage("variable_list::add_item() trying to re-add item named \"integer_var\"."));

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t clone(list->clone("clone"));
        CATCH_REQUIRE(clone != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_list::pointer_t l2(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_list>(clone));
        CATCH_REQUIRE(l2 != nullptr);

        CATCH_REQUIRE(l2->get_item(-1) == nullptr);
        CATCH_REQUIRE(l2->get_item(2) == nullptr);

        // the items are also cloned so we can quickly test that they are not
        // equal and then we can verify the type or the name
        //
        CATCH_REQUIRE(l2->get_item(0) != var1);
        CATCH_REQUIRE(l2->get_item(1) != var1);
        CATCH_REQUIRE(l2->get_item(0) != var2);
        CATCH_REQUIRE(l2->get_item(1) != var2);

        CATCH_REQUIRE(l2->get_item(0)->get_type() == "integer");
        CATCH_REQUIRE(l2->get_item(1)->get_type() == "void");

        // make sure original is still valid
        //
        CATCH_REQUIRE(l->get_item(0) == var2);
        CATCH_REQUIRE(l->get_item(-1) == nullptr);
        CATCH_REQUIRE(l->get_item(1) == var1);
        CATCH_REQUIRE(l->get_item(2) == nullptr);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_executor_error", "[executor][reporter][error]")
{
    CATCH_START_SECTION("if() before any condition")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("if_too_soon", g_program_no_condition));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "trying to use a 'compare' result when none are currently defined."));

        CATCH_REQUIRE_THROWS_MATCHES(
              s->set_compare(SNAP_CATCH2_NAMESPACE::reporter::compare_t::COMPARE_UNDEFINED)
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "'compare' cannot be set to \"undefined\"."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("exit() + error message")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("exit_error_message", g_program_error_message));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();

        CATCH_REQUIRE(s->get_exit_code() == 1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("listen() + listen()")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("two_listen", g_program_two_listen));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "the listen() instruction cannot be reused without an intermediate disconnect() instruction."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("label(name: ...) does not accept integers")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("label_bad_type", g_program_label_bad_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));

        // label is a special case which we test in the state.cpp way before
        // we reach the executor... (so this is not really an executor test)
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "the value of the \"name\" parameter of the \"label\" statement must be an identifier."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("exit(error_message: ...) does not accept floating points")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("exit_bad_type", g_program_exit_bad_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "parameter type mismatch for error_message, expected \"string\", got \"floating_point\" instead."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify starting the thread twice")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_start_thread_twice", g_program_start_thread_twice));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 4);

        // before we run the script, there are no such variables
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("test"));
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("runner");
        CATCH_REQUIRE(var == nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();
        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "run() instruction found when already running in the background."));

        var = s->get_variable("test");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "test");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 33);

        var = s->get_variable("runner");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "runner");
        CATCH_REQUIRE(var->get_type() == "floating_point");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_floating_point>(var)->get_floating_point() == 6.07);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("<type> + <type> that are not valid")
    {
        constexpr char const * const bad_additions[] =
        {
            g_program_unsupported_addition_address_address,
            g_program_unsupported_addition_address_string,
            g_program_unsupported_addition_string_address,
            g_program_unsupported_addition_address_identifier,
            g_program_unsupported_addition_identifier_address,
            g_program_unsupported_addition_identifier_string,
            g_program_unsupported_addition_string_identifier,
        };

        for(auto const & program : bad_additions)
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("<type> + <type> that is not valid", program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , std::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "unsupported addition."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("<type> - <type> that are not valid")
    {
        constexpr char const * const bad_subtractions[] =
        {
            g_program_unsupported_subtraction_address_string,
            g_program_unsupported_subtraction_string_address,
            g_program_unsupported_subtraction_address_identifier,
            g_program_unsupported_subtraction_identifier_address,
            g_program_unsupported_subtraction_identifier_string,
            g_program_unsupported_subtraction_string_identifier,
        };

        for(auto const & program : bad_subtractions)
        {
//std::cerr << "testing [" << program << "]\n";
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("<type> - <type> that is not valid", program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , std::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "unsupported subtraction."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("<type> * <type> that are not valid")
    {
        constexpr char const * const bad_multiplications[] =
        {
            g_program_unsupported_multiplication_address_address,
            g_program_unsupported_multiplication_address_string,
            g_program_unsupported_multiplication_string_address,
            g_program_unsupported_multiplication_address_identifier,
            g_program_unsupported_multiplication_identifier_address,
            g_program_unsupported_multiplication_identifier_string,
            g_program_unsupported_multiplication_string_identifier,
            g_program_unsupported_multiplication_string_string,
            g_program_unsupported_multiplication_identifier_identifier,
        };

        for(auto const & program : bad_multiplications)
        {
//std::cerr << "testing [" << program << "]\n";
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("<type> * <type> that is not valid", program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , std::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "unsupported multiplication."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("<type> / <type> that are not valid")
    {
        constexpr char const * const bad_divisions[] =
        {
            g_program_unsupported_division_address_address,
            g_program_unsupported_division_address_string,
            g_program_unsupported_division_string_address,
            g_program_unsupported_division_address_identifier,
            g_program_unsupported_division_identifier_address,
            g_program_unsupported_division_identifier_string,
            g_program_unsupported_division_string_identifier,
            g_program_unsupported_division_string_string,
            g_program_unsupported_division_identifier_identifier,
        };

        for(auto const & program : bad_divisions)
        {
//std::cerr << "testing [" << program << "]\n";
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("<type> / <type> that is not valid", program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , std::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "unsupported division."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("<type> % <type> that are not valid")
    {
        constexpr char const * const bad_divisions[] =
        {
            g_program_unsupported_modulo_address_address,
            g_program_unsupported_modulo_address_string,
            g_program_unsupported_modulo_string_address,
            g_program_unsupported_modulo_address_identifier,
            g_program_unsupported_modulo_identifier_address,
            g_program_unsupported_modulo_identifier_string,
            g_program_unsupported_modulo_string_identifier,
            g_program_unsupported_modulo_string_string,
            g_program_unsupported_modulo_identifier_identifier,
        };

        for(auto const & program : bad_divisions)
        {
//std::cerr << "testing [" << program << "]\n";
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("<type> % <type> that is not valid", program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , std::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "unsupported modulo."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("-<types> that are not valid")
    {
        constexpr char const * const bad_negations[] =
        {
            g_program_unsupported_negation_single_string,
            g_program_unsupported_negation_double_string,
            g_program_unsupported_negation_address,
        };

        for(auto const & program : bad_negations)
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("-<type> is not valid", program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , std::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "unsupported negation."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("<string> * <negative> is not valid")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("<string> * <negative> is not valid", g_program_unsupported_negation_repeat));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "string repeat needs to be positive and under 1001."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("<string> * <large repeat> is not valid")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("<string> * <large repeat> is not valid", g_program_unsupported_large_repeat));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "string repeat needs to be positive and under 1001."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("exit() with timeout & error_message is invalid")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("bad_exit.vpr", g_program_bad_exit));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "\"timeout\" and \"error_message\" from the exit() instruction are mutually exclusive."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("exit() with timeout which is not a number")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("bad_exit.vpr", g_program_bad_exit_timeout));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "parameter type mismatch for timeout, expected \"number\", got \"string\" instead."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("send_message() when not connected")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("bad_send_message.vpr", g_program_send_message_without_connection));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "send_message() has no connection to send a message to."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("if(variable) with invalid type")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("if_invalid_type.vpr", g_program_if_invalid_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 5);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "if(variable: ...) only supports variables of type integer or floating point."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("wait() before starting thread")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("wait_outside_thread.vpr", g_program_wait_outside_thread));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "wait() used before run()."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("wait() with invalid mode")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_wait_invalid_mode", g_program_wait_invalid_mode));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        addr::addr a;
        sockaddr_in ip = {
            .sin_family = AF_INET,
            .sin_port = htons(20002),
            .sin_addr = {
                .s_addr = htonl(0x7f000001),
            },
            .sin_zero = {},
        };
        a.set_ipv4(ip);
        messenger_responder::pointer_t messenger(std::make_shared<messenger_responder>(
                  a
                , ed::mode_t::MODE_PLAIN
                , messenger_responder::sequence_t::SEQUENCE_ONE_MESSAGE));
        ed::communicator::instance()->add_connection(messenger);
        messenger_timer::pointer_t timer(std::make_shared<messenger_timer>(messenger));
        ed::communicator::instance()->add_connection(timer);
        messenger->set_timer(timer);
        e->set_thread_done_callback([messenger, timer]()
            {
                ed::communicator::instance()->remove_connection(messenger);
                ed::communicator::instance()->remove_connection(timer);
            });

        e->run();

        // the thread exception happens when e->stop() is called
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage("unknown mode \"not_this_one\" in wait()."));

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("wait() + drain without connections")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_wait_no_connection", g_program_wait_no_connections));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        // the thread exception happens when e->stop() is called
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage("no connections to wait() on."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_executor_error_message", "[executor][reporter][error]")
{
    CATCH_START_SECTION("verify message fails")
    {
        struct bad_verification_t
        {
            char const * const  f_program = nullptr;
            char const * const  f_error = nullptr;
        };
        bad_verification_t const bad_verifications[] =
        {
            {
                g_program_verify_message_fail_sent_server,
                "message expected sent from server name \"not_this_one\" did not match \"\".",
            },
            {
                g_program_verify_message_fail_sent_service,
                "message expected sent from service name \"not_this_one\" did not match \"\".",
            },
            {
                g_program_verify_message_fail_server,
                "message expected server name \"not_this_one\" did not match \"\".",
            },
            {
                g_program_verify_message_fail_service,
                "message expected service name \"not_this_one\" did not match \"\".",
            },
            {
                g_program_verify_message_fail_command,
                "message expected command \"NOT_THIS_ONE\" did not match \"REGISTER\".",
            },
            {
                g_program_verify_message_fail_forbidden,
                "message forbidden parameter \"version\" was found in this message.",
            },
            {
                g_program_verify_message_fail_required,
                "message required parameter \"not_this_one\" was not found in this message.",
            },
            {
                g_program_verify_message_fail_required_int_value,
                "message expected parameter \"version\" to be an integer set to \"200\" but found \"1\" instead.",
            },
            {
                g_program_verify_message_fail_required_str_value,
                "message expected parameter \"service\" to be a string set to \"not_this_one\" but found \"responder\" instead.",
            },
            {
                g_program_verify_message_fail_required_flt_value,
                "message parameter type \"floating_point\" not supported yet.",
            },
        };
        for(auto const & bv : bad_verifications)
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_verify_message_fail", bv.f_program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            e->start();
            addr::addr a;
            sockaddr_in ip = {
                .sin_family = AF_INET,
                .sin_port = htons(20002),
                .sin_addr = {
                    .s_addr = htonl(0x7f000001),
                },
                .sin_zero = {},
            };
            a.set_ipv4(ip);
            messenger_responder::pointer_t messenger(std::make_shared<messenger_responder>(
                      a
                    , ed::mode_t::MODE_PLAIN
                    , messenger_responder::sequence_t::SEQUENCE_ONE_MESSAGE));
            ed::communicator::instance()->add_connection(messenger);
            messenger_timer::pointer_t timer(std::make_shared<messenger_timer>(messenger));
            ed::communicator::instance()->add_connection(timer);
            messenger->set_timer(timer);
            e->set_thread_done_callback([messenger, timer]()
                {
                    ed::communicator::instance()->remove_connection(messenger);
                    ed::communicator::instance()->remove_connection(timer);
                });

            e->run();

            // the thread exception happens when e->stop() is called
            //
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->stop()
                , std::runtime_error
                , Catch::Matchers::ExceptionMessage(bv.f_error));

            // if we exited because of our timer, then the test did not pass
            //
            CATCH_REQUIRE_FALSE(timer->timed_out_prima());

            CATCH_REQUIRE(s->get_exit_code() == -1);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("wait for nothing (should time out)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_wait_for_nothing", g_program_wait_for_nothing));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        addr::addr a;
        sockaddr_in ip = {
            .sin_family = AF_INET,
            .sin_port = htons(20002),
            .sin_addr = {
                .s_addr = htonl(0x7f000001),
            },
            .sin_zero = {},
        };
        a.set_ipv4(ip);
        messenger_responder::pointer_t messenger(std::make_shared<messenger_responder>(
                  a
                , ed::mode_t::MODE_PLAIN
                , messenger_responder::sequence_t::SEQUENCE_ONE_MESSAGE));
        ed::communicator::instance()->add_connection(messenger);
        messenger_timer::pointer_t timer(std::make_shared<messenger_timer>(messenger));
        ed::communicator::instance()->add_connection(timer);
        messenger->set_timer(timer);
        e->set_thread_done_callback([messenger, timer]()
            {
                ed::communicator::instance()->remove_connection(messenger);
                ed::communicator::instance()->remove_connection(timer);
            });
        e->run();

        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage("ppoll() timed out."));

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
