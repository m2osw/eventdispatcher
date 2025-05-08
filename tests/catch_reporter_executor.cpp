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

#include    <eventdispatcher/reporter/instruction_factory.h>
#include    <eventdispatcher/reporter/parser.h>
#include    <eventdispatcher/reporter/variable_address.h>
#include    <eventdispatcher/reporter/variable_floating_point.h>
#include    <eventdispatcher/reporter/variable_integer.h>
#include    <eventdispatcher/reporter/variable_list.h>
#include    <eventdispatcher/reporter/variable_regex.h>
#include    <eventdispatcher/reporter/variable_string.h>
#include    <eventdispatcher/reporter/variable_timestamp.h>
#include    <eventdispatcher/reporter/variable_void.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/exception.h>
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>


// snapdev
//
#include    <snapdev/gethostname.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{



constexpr char const * const g_program_sleep_func =
    "call(label: func_sleep)\n"
    "exit(timeout: 1)\n"
    "label(name: func_sleep)\n"
    "sleep(seconds: 2.5)\n"
    "return()\n"
;

constexpr char const * const g_program_sort_func =
    "set_variable(name: s1, value: 'hello')\n"
    "set_variable(name: s2, value: 'world')\n"
    "set_variable(name: s3, value: 'who')\n"
    "set_variable(name: s4, value: 'are')\n"
    "set_variable(name: s5, value: 'you?')\n"
    "sort(var1: s1, var2: s2, var3: s3, var4: s4, var5: s5)\n"

    "set_variable(name: i1, value: 506)\n"
    "set_variable(name: i2, value: 1003)\n"
    "set_variable(name: i3, value: 73)\n"
    "set_variable(name: i4, value: 1004)\n"
    "set_variable(name: i5, value: -3)\n"
    "sort(var1: i1, var2: i2, var3: i3, var4: i4, var5: i5)\n"

    "set_variable(name: f1, value: 50.6)\n"
    "set_variable(name: f2, value: -10.103)\n"
    "set_variable(name: f3, value: 73.5)\n"
    "set_variable(name: f4, value: 1.004)\n"
    "set_variable(name: f5, value: -0.3)\n"
    "sort(var1: f1, var2: f2, var3: f3, var4: f4, var5: f5)\n"

    "exit()\n"
;

constexpr char const * const g_program_start_thread =
    "set_variable(name: test, value: 33)\n"
    "set_variable(name: test_copy_between_dollars, value: \"$${test}$\")\n"
    "run()\n"
    "set_variable(name: runner, value: 6.07)\n"
    "set_variable(name: runner_copy_as_is, value: \"runner = ${runner}\", type: string)\n"
    "set_variable(name: time_limit, value: @1713934141.107805991, type: timestamp)\n"
    "set_variable(name: time_limit_copy, value: \"limit: ${time_limit}\")\n"
    "set_variable(name: time_from_float, value: \"1713934141.107805991\", type: timestamp)\n"
    "set_variable(name: host_ip, value: <127.7.3.51>)\n"
    "set_variable(name: host_ip_copy, value: \"Host is at ${host_ip} address\")\n"
    "set_variable(name: time_and_host_ip, value: \"time ${time_limit} and address ${host_ip}...\")\n"
    "strlen(variable_name: length, string: ${time_and_host_ip})\n"
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

    "set_variable(name: t31, value: @300.561 - @123.231)\n"
    "set_variable(name: t32, value: @34.3123 + @123.9984312)\n"
;

constexpr char const * const g_program_verify_hex =
    "hex(variable_name: t01, value: 0x1a4fd2)\n"
    "hex(variable_name: t02, value: 0xabcdef, uppercase: 0)\n"
    "hex(variable_name: t03, value: 0xabcdef, uppercase: 1)\n"
    "hex(variable_name: t04, value: 1, width: 8)\n"
    "hex(variable_name: t05, value: 0xabcdef, uppercase: 1, width: 8)\n"
;

constexpr char const * const g_program_verify_now =
    "now(variable_name: about_now)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_max_pid =
    "max_pid(variable_name: top_pid)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_random =
    "random(variable_name: any_number)\n"
    "random(variable_name: positive, negative: 0)\n"
    "random(variable_name: positive_or_negative, negative: 1)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_hostname =
    "hostname(variable_name: host_name)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_kill_number =
    "kill(signal: 18)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_kill_identifier =
    "kill(signal: SIGCONT)\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_kill_string =
    "kill(signal: \"cont\")\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_kill_unsupported_timestamp =
    "kill(signal: @123.3342)\n"

    "exit(error_message: \"test is expected to fail before reaching this staement.\")\n"
;

constexpr char const * const g_program_verify_kill_integer_too_large =
    "kill(signal: 100)\n"

    "exit(error_message: \"test is expected to fail before reaching this staement.\")\n"
;

constexpr char const * const g_program_verify_kill_unknown_signal_name =
    "kill(signal: \"unknown\")\n"

    "exit(error_message: \"test is expected to fail before reaching this staement.\")\n"
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

    "set_variable(name: t41, value: 'single' + `[0-9]+`)\n"
    "set_variable(name: t42, value: `[0-9]+` + 'single')\n"
    "set_variable(name: t43, value: \"string\" + `[0-9]+`)\n"
    "set_variable(name: t44, value: `[0-9]+` + \"double\")\n"
    "set_variable(name: t45, value: 'a|b' + `[0-9]+`)\n"
    "set_variable(name: t46, value: `[0-9]+` + 'c{3,9}')\n"
    "set_variable(name: t47, value: \"[a-z]?\" + `[0-9]+`)\n"
    "set_variable(name: t48, value: `[0-9]+` + \"a?b?c?\")\n"
    "set_variable(name: t49, value: `[0-9]+` + `(a|b|c)?`)\n"
;

constexpr char const * const g_program_verify_computation_string_repeat =
    "set_variable(name: t01, value: 'abc' * 3)\n"
    "set_variable(name: t02, value: \"xyz\" * 5)\n"
    "set_variable(name: t03, value: \"zero\" * 0)\n"
    "set_variable(name: t04, value: \"one\" * 1)\n"
;

constexpr char const * const g_program_verify_variable_in_string =
    "set_variable(name: foo, value: 'abc')\n"
    "set_variable(name: bar, value: \"[${foo}]\")\n"
;

constexpr char const * const g_program_accept_one_message =
    "run()\n"
    "listen(address: <127.0.0.1:20002>, connection_type: messenger)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n" // first wait reacts on connect(), second wait receives the REGISTER message
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: `^REGISTER$`, required_parameters: { service: responder, version: 1 }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "save_parameter_value(parameter_name: command, variable_name: command)\n"
    "save_parameter_value(parameter_name: version, variable_name: register_version)\n"
    "save_parameter_value(parameter_name: service, variable_name: register_service, type: identifier)\n"
    "send_message(command: READY, sent_server: reporter_test_extension, sent_service: test_processor, server: reporter_test, service: accept_one_message, parameters: { status: alive, version: 9 })\n"
    "wait(timeout: 10.0, mode: drain)\n"
    "disconnect()\n"
    "exit()\n"
;

constexpr char const * const g_program_receive_unwanted_message =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: `^[0-9]+$` }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "save_parameter_value(parameter_name: version, variable_name: register_version, type: integer)\n"
    "send_message(command: READY, parameters: { version: 9 })\n"
    "print(message: \"nearly done\")\n"
    "exit(timeout: 2.5)\n"
;

constexpr char const * const g_program_send_unsupported_message_parameter_type =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n" // first wait reacts on connect(), second wait receives the REGISTER message
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: 1 }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "save_parameter_value(parameter_name: version, variable_name: register_version, type: integer)\n"
    "send_message(command: READY, parameters: { status: 3.05 })\n" // floating point not yet supported
    "wait(timeout: 1.0, mode: drain)\n"
;

constexpr char const * const g_program_send_invalid_parameter_value_type =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: 1 }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "save_parameter_value(parameter_name: service, variable_name: register_version, type: integer)\n" // service is not an integer
    "send_message(command: READY, parameters: { status: \"3.05\" })\n"
    "wait(timeout: 1.0, mode: drain)\n"
;

constexpr char const * const g_program_save_parameter_of_type_timestamp =
    "run()\n"
    "listen(address: <127.0.0.1:20002>, connection_type: messenger)\n"
    "label(name: wait_message)\n"
    "wait(timeout: 10.0, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: 1 }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "save_parameter_value(parameter_name: version, variable_name: register_version, type: integer)\n"
    "save_parameter_value(parameter_name: large_number, variable_name: default_integer, type: integer)\n"
    "send_message(command: READY, parameters: { status: \"3.05\", date: @1715440881.543723981 })\n"
    "clear_message()\n"
    "label(name: wait_second_message)\n"
    "wait(timeout: 10.0, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_second_message)\n"
    "show_message()\n"
    "verify_message(command: TIMED, required_parameters: { now: `^[0-9]+(\\\\.[0-9]+)?$` } )\n"
    "save_parameter_value(parameter_name: now, variable_name: timed_value, type: timestamp)\n"
    "verify_message(command: TIMED, required_parameters: { now: ${timed_value} } )\n" // this time we try with a timestamp parameter
    "save_parameter_value(parameter_name: not_defined, variable_name: default_time, type: timestamp)\n"
    "exit()\n"
;

constexpr char const * const g_program_save_parameter_with_unknown_type =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: 1 }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "save_parameter_value(parameter_name: service, variable_name: register_version, type: void)\n" // "void" is not a supported type here
    "send_message(command: READY, parameters: { status: \"3.05\" })\n"
    "wait(timeout: 1.0, mode: drain)\n"
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

constexpr char const * const g_program_compare_and_if =
    // integer greater/less
    "set_variable(name: a, value: 13)\n"
    "set_variable(name: b, value: 10)\n"
    "compare(expression: ${a} <=> ${b})\n"
    "if(greater: integer_greater)\n"
    "exit(error_message: \"if(greater: 13 <=> 10) failed test.\")\n"
    "label(name: integer_greater)\n"

    "compare(expression: ${b} <=> ${a})\n"
    "if(less: integer_less)\n"
    "exit(error_message: \"if(less: 10 <=> 13) failed test.\")\n"
    "label(name: integer_less)\n"

    "compare(expression: ${a} <=> ${a})\n"
    "if(equal: integer_equal)\n"
    "exit(error_message: \"if(equal: 13 <=> 13) failed test.\")\n"
    "label(name: integer_equal)\n"

    // floating point greater/less
    "set_variable(name: c, value: 13.41)\n"
    "set_variable(name: d, value: 9.05)\n"
    "compare(expression: ${c} <=> ${d})\n"
    "if(greater: floating_point_greater)\n"
    "exit(error_message: \"if(greater: 13.41 <=> 9.05) failed test.\")\n"
    "label(name: floating_point_greater)\n"

    "compare(expression: ${d} <=> ${c})\n"
    "if(less: floating_point_less)\n"
    "exit(error_message: \"if(less: 9.05 <=> 13.41) failed test.\")\n"
    "label(name: floating_point_less)\n"

    "compare(expression: ${d} <=> ${d})\n"
    "if(equal: floating_point_equal)\n"
    "exit(error_message: \"if(equal: 9.05 <=> 9.05) failed test.\")\n"
    "label(name: floating_point_equal)\n"

    // floating point vs integer greater/less
    "set_variable(name: c2, value: 13.0)\n"
    "set_variable(name: d2, value: 10.0)\n"
    "compare(expression: ${a} <=> ${d})\n"
    "if(greater: integer_floating_point_greater)\n"
    "exit(error_message: \"if(greater: 13 <=> 9.05) failed test.\")\n"
    "label(name: integer_floating_point_greater)\n"

    "compare(expression: ${d} <=> ${b})\n"
    "if(less: floating_point_integer_less)\n"
    "exit(error_message: \"if(less: 9.05 <=> 10) failed test.\")\n"
    "label(name: floating_point_integer_less)\n"

    "compare(expression: ${d2} <=> ${b})\n"
    "if(equal: floating_point_integer_equal)\n"
    "exit(error_message: \"if(equal: 10.0 <=> 10) failed test.\")\n"
    "label(name: floating_point_integer_equal)\n"

    "compare(expression: ${a} <=> ${c2})\n"
    "if(equal: integer_floating_point_equal)\n"
    "exit(error_message: \"if(equal: 10 <=> 10.0) failed test.\")\n"
    "label(name: integer_floating_point_equal)\n"

    "compare(expression: ${b} <=> ${c2})\n"
    "if(less: integer_floating_point_less)\n"
    "exit(error_message: \"if(less: 10 <=> 13.0) failed test.\")\n"
    "label(name: integer_floating_point_less)\n"

    "compare(expression: ${c} <=> ${a})\n"
    "if(greater: floating_point_integer_greater)\n"
    "exit(error_message: \"if(greater: 13.41 <=> 13) failed test.\")\n"
    "label(name: floating_point_integer_greater)\n"

    // timestamp greater/less (same as integers)
    "now(variable_name: now)\n"
    "set_variable(name: e, value: ${now} + 0.3)\n"
    "set_variable(name: f, value: ${now} - 0.05)\n"
    "compare(expression: ${e} <=> ${f})\n"
    "if(greater: timestamp_greater)\n"
    "exit(error_message: \"if(greater: now + 0.3 <=> now - 0x05) failed test.\")\n"
    "label(name: timestamp_greater)\n"

    "compare(expression: ${f} <=> ${e})\n"
    "if(less: timestamp_less)\n"
    "exit(error_message: \"if(less: now - 0.05 <=> now + 0.3) failed test.\")\n"
    "label(name: timestamp_less)\n"

    "compare(expression: ${f} <=> ${f})\n"
    "if(equal: timestamp_equal)\n"
    "exit(error_message: \"if(equal: now - 0.05 <=> now - 0.05) failed test.\")\n"
    "label(name: timestamp_equal)\n"

    // double strings greater/less/equal
    "set_variable(name: g, value: \"str9\")\n"
    "set_variable(name: h, value: \"str2\")\n"
    "compare(expression: ${g} <=> ${h})\n"
    "if(greater: double_string_greater)\n"
    "exit(error_message: \"if(greater: \\\"str9\\\" <=> \\\"str2\\\") failed test.\")\n"
    "label(name: double_string_greater)\n"

    "compare(expression: ${h} <=> ${g})\n"
    "if(less: double_string_less)\n"
    "exit(error_message: \"if(less: \\\"str2\\\" <=> \\\"str9\\\") failed test.\")\n"
    "label(name: double_string_less)\n"

    "compare(expression: ${h} <=> ${h})\n"
    "if(equal: double_string_equal)\n"
    "exit(error_message: \"if(equal: \\\"str2\\\" <=> \\\"str2\\\") failed test.\")\n"
    "label(name: double_string_equal)\n"

    // single strings greater/less
    "set_variable(name: i, value: 'str8')\n"
    "set_variable(name: j, value: 'str5')\n"
    "compare(expression: ${i} <=> ${j})\n"
    "if(greater: single_string_greater)\n"
    "exit(error_message: \"if(greater: 'str9' <=> 'str2') failed test.\")\n"
    "label(name: single_string_greater)\n"

    "compare(expression: ${j} <=> ${i})\n"
    "if(less: single_string_less)\n"
    "exit(error_message: \"if(less: 'str2' <=> 'str9') failed test.\")\n"
    "label(name: single_string_less)\n"

    // mixed strings greater/less
    "compare(expression: ${g} <=> ${j})\n"
    "if(greater: mixed_string_greater)\n"
    "exit(error_message: \"if(greater: \\\"str9\\\" <=> 'str5') failed test.\")\n"
    "label(name: mixed_string_greater)\n"

    "compare(expression: ${i} <=> ${g})\n"
    "if(less: mixed_string_less)\n"
    "exit(error_message: \"if(less: 'str8' <=> \\\"str9\\\") failed test.\")\n"
    "label(name: mixed_string_less)\n"

    // address greater/less
    "set_variable(name: k, value: <127.0.0.100>)\n"
    "set_variable(name: l, value: <10.127.0.100>)\n"
    "compare(expression: ${k} <=> ${l})\n"
    "if(greater: address_greater)\n"
    "exit(error_message: \"if(greater: <127.0.0.100> <=> <10.127.0.100>) failed test.\")\n"
    "label(name: address_greater)\n"

    "compare(expression: ${l} <=> ${k})\n"
    "if(less: address_less)\n"
    "exit(error_message: \"if(less: <10.127.0.100> <=> <127.0.0.100>) failed test.\")\n"
    "label(name: address_less)\n"

    "compare(expression: ${l} <=> ${l})\n"
    "if(equal: address_equal)\n"
    "exit(error_message: \"if(equal: <10.127.0.100> <=> <10.127.0.100>) failed test.\")\n"
    "label(name: address_equal)\n"

    "exit()\n"
;

constexpr char const * const g_program_compare_with_incompatible_types =
    // cannot compare an integer vs string
    "set_variable(name: a, value: 13)\n"
    "set_variable(name: b, value: 'a string')\n"
    "compare(expression: ${a} <=> ${b})\n"

    "exit(error_message: \"test is expected to fail before reaching this staement.\")\n"
;

constexpr char const * const g_program_compare_with_non_integer =
    // compare expression must be an integer
    "compare(expression: 'string')\n"

    "exit(error_message: \"test is expected to fail before reaching this staement.\")\n"
;

constexpr char const * const g_program_compare_with_bad_positive_integer =
    // compare expression must be between -2 and +1
    "compare(expression: 5)\n"

    "exit(error_message: \"test is expected to fail before reaching this staement.\")\n"
;

constexpr char const * const g_program_compare_with_bad_negative_integer =
    // compare expression must be between -2 and +1
    "compare(expression: -10)\n"

    "exit(error_message: \"test is expected to fail before reaching this staement.\")\n"
;

constexpr char const * const g_program_print_message =
    "print(message: \"testing print()\")\n"
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
    "set_variable(name: bad, value: invalid % \"invalid\")\n"
;

constexpr char const * const g_program_unsupported_modulo_string_identifier =
    "set_variable(name: bad, value: 'invalid' % invalid)\n"
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

constexpr char const * const g_program_unterminated_double_string_variable =
    "set_variable(name: my_var, value: \"blah\")\n"
    "set_variable(name: missing_close, value: \"ref. ${my_var\")\n"
;

constexpr char const * const g_program_regex_in_double_string_variable =
    "set_variable(name: my_regex, value: `[a-z]+`)\n"
    "set_variable(name: missing_close, value: \"ref. ${my_regex}\")\n"
;

constexpr char const * const g_program_primary_variable_references =
    "set_variable(name: my_string_var, value: \"foo\")\n"
    "set_variable(name: longer_string_var, value: ${my_string_var})\n"
    "set_variable(name: my_integer_var, value: 41)\n"
    "set_variable(name: longer_integer_var, value: ${my_integer_var})\n"
    "set_variable(name: my_floating_point_var, value: 303.601)\n"
    "set_variable(name: longer_floating_point_var, value: ${my_floating_point_var})\n"
    "set_variable(name: my_identifier_var, value: bar)\n"
    "set_variable(name: longer_identifier_var, value: ${my_identifier_var})\n"
    "set_variable(name: my_regex_var, value: `^[regex]$`)\n"
    "set_variable(name: longer_regex_var, value: ${my_regex_var})\n"
    "set_variable(name: my_address_var, value: <10.12.14.16:89>)\n"
    "set_variable(name: longer_address_var, value: ${my_address_var})\n"
    "set_variable(name: my_timestamp_var, value: @1714241733.419438123)\n"
    "set_variable(name: longer_timestamp_var, value: ${my_timestamp_var})\n"
;

constexpr char const * const g_program_wrong_primary_variable_reference =
    "set_variable(name: my_var, value: foo)\n"
    "set_variable(name: longer_var, value: ${wrong_name})\n"
;

constexpr char const * const g_program_double_string_variable_without_name =
    "set_variable(name: missing_close, value: \"ref. ${} is empty\")\n"
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

constexpr char const * const g_program_bad_print_message =
    "print(message: string_expected)\n"
;

constexpr char const * const g_program_send_message_without_connection =
    "send_message(server: \"world\", service: cluckd, command: WITHOUT_CONNECTION)\n"
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

constexpr char const * const g_program_invalid_string_to_timestamp_cast =
    "set_variable(name: time_limit, value: '1713b34141.10780g991', type: timestamp)\n"
;

constexpr char const * const g_program_unknown_string_cast =
    "set_variable(name: time_limit, value: 'not important', type: unknown)\n"
;

constexpr char const * const g_program_unknown_timestamp_cast =
    "set_variable(name: time_limit, value: @123.456, type: unknown)\n"
;

constexpr char const * const g_program_unknown_source_cast =
    "set_variable(name: time_limit, value: <127.127.127.127>, type: string)\n"
;

constexpr char const * const g_program_sort_var1_missing =
    "set_variable(name: s2, value: 'err33')\n"
    "set_variable(name: s3, value: 'err13')\n"
    "sort(var2: s2, var3: s3)\n"
;

constexpr char const * const g_program_sort_var1_not_string =
    "sort(var1: 33)\n"
;

constexpr char const * const g_program_sort_var1_not_found =
    "sort(var1: not_defined)\n"
;

constexpr char const * const g_program_sort_wrong_type =
    // note that with time we are likely to support all types and thus
    // this test may stop working
    //
    "set_variable(name: w1, value: <127.0.0.1>)\n"
    "sort(var1: w1)\n"
;

constexpr char const * const g_program_sort_mixed_types =
    "set_variable(name: s1, value: 'err13')\n"
    "set_variable(name: s2, value: 33)\n"
    "set_variable(name: s3, value: 'more')\n"
    "sort(var1: s1, var2: s2, var3: s3)\n"
;

constexpr char const * const g_program_listen_with_unknown_connection_type =
    "listen(address: <127.0.0.1:20002>, connection_type: unknown)\n"
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

constexpr char const * const g_program_verify_message_fail_required_long_str_value =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: 'responder' * 15 })\n"
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

constexpr char const * const g_program_verify_message_fail_required_timestamp_value =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { version: @123 })\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_timestamp_command =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: @123.678, required_parameters: { version: 1 })\n"
    "exit()\n"
;

constexpr char const * const g_program_verify_message_fail_unexpected_command =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 12, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: `^NOT_THIS_ONE$`, required_parameters: { version: 1 })\n"
    "exit()\n"
;

constexpr char const * const g_program_last_wait =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: 1 }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
    "send_message(command: READY, sent_server: reporter_test_extension, sent_service: test_processor, server: reporter_test, service: accept_one_message, parameters: { status: alive })\n"
    "wait(timeout: 1.0, mode: drain)\n" // hope that send_message() is small enough that a single wait is sufficient to send it
    "wait(timeout: 1.0)\n"
    "disconnect()\n"
    "exit()\n"
;

constexpr char const * const g_program_regex_parameter_no_match =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0, mode: wait)\n"
    "has_message()\n"
    "if(false: wait_message)\n"
    "show_message()\n"
    "verify_message(command: REGISTER, required_parameters: { service: responder, version: `_[a-z]+` }, optional_parameters: { commands: \"READY,HELP,STOP\" }, forbidden_parameters: { forbidden })\n"
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

constexpr char const * const g_program_wait_for_timeout =
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
    "wait(timeout: 1.0, mode: timeout)\n" // one extra wait(), which will time out
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
        .f_name = "set_variable",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_AFTER_CALL,
        .f_name = "set_variable",
    },
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
        .f_name = "set_variable",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_AFTER_CALL,
        .f_name = "set_variable",
    },
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
        .f_name = "set_variable",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_AFTER_CALL,
        .f_name = "set_variable",
    },
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
        .f_name = "strlen",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_AFTER_CALL,
        .f_name = "strlen",
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
        SEQUENCE_TIMED_MESSAGE,
        SEQUENCE_READY_HELP_MESSAGE,
        SEQUENCE_READY_THROW,
        SEQUENCE_READY_THROW_WHAT,
    };

    messenger_responder(
              addr::addr const & a
            , ed::mode_t mode
            , sequence_t sequence
            , int timeout = 500'000) // or ed::DEFAULT_PAUSE_BEFORE_RECONNECTING
        : tcp_client_permanent_message_connection(
              a
            , mode
            , timeout
            , true
            , "responder")  // service name
        , f_sequence(sequence)
    {
        set_name("messenger_responder");    // connection name
        set_timeout_delay(500'000);         // 0.5 seconds
    }

    virtual void process_connected() override
    {
        // always register at the time we connect
        //
        tcp_client_permanent_message_connection::process_connected();
        register_service();
    }

    virtual void process_message(ed::message & msg) override
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
            if(msg.has_parameter("version"))
            {
                // there are cases where I put a version as an integer
                //
                std::int64_t version(msg.get_integer_parameter("version"));
                if(version != 9)
                {
                    throw std::runtime_error(
                          "READY version value invalid; expected 9, got "
                        + std::to_string(version)
                        + " instead.");
                }
            }
            if(msg.has_parameter("date"))
            {
                // there are cases where I put a date as a timespec_ex (a timestamp in the language)
                //
                snapdev::timespec_ex date(msg.get_timespec_parameter("date"));
                if(date != snapdev::timespec_ex(1715440881, 543723981))
                {
                    throw std::runtime_error(
                          "READY date value invalid; expected 1715440881.543723981, got "
                        + date.to_timestamp()
                        + " instead.");
                }
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
            }
            break;

        case sequence_t::SEQUENCE_TIMED_MESSAGE:
            {
                ed::message unwanted;
                unwanted.reply_to(msg);
                unwanted.set_command("TIMED");
                unwanted.add_parameter("now", snapdev::now());
                if(!send_message(unwanted, false))
                {
                    throw std::runtime_error("could not send TIMED message");
                }
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

        case sequence_t::SEQUENCE_READY_THROW:
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

                // got the help message, now do a "legitimate" throw
                //
                throw std::runtime_error("testing that the executor catches these exceptions.");

            default:
                throw std::runtime_error("reached step 4 of SEQUENCE_READY_THROW?");

            }
            break;

        case sequence_t::SEQUENCE_READY_THROW_WHAT:
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

                // got the help message, now do a "legitimate" throw
                //
                struct my_exception
                {
                    int code = 0;
                };
                throw my_exception({5});

            default:
                throw std::runtime_error("reached step 4 of SEQUENCE_READY_THROW_WHAT?");

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
        // call default function(s)
        //
        timer::process_timeout();

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
    CATCH_START_SECTION("reporter_executor: verify sleep in a function")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_sleep_func.rprtr", g_program_sleep_func));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 5);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        snapdev::timespec_ex const start(snapdev::now());
        e->start();
        CATCH_REQUIRE(e->run());
        snapdev::timespec_ex const duration(snapdev::now() - start);
        CATCH_REQUIRE(duration.tv_sec >= 2); // we slept for 2.5 seconds, so we expect at least start + 2 seconds
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify sort function")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_sleep_func.rprtr", g_program_sort_func));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 19);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);

        // *** STRING ***
        struct verify_string_t
        {
            char const *            f_name = nullptr;
            char const *            f_value = nullptr;
        };

        // verify that the resulting set of variable is indeed sorted
        verify_string_t string_verify[] =
        {
            { "s1", "are"   },
            { "s2", "hello" },
            { "s3", "who"   },
            { "s4", "world" },
            { "s5", "you?"  },
        };

        for(auto const & v : string_verify)
        {
//std::cerr << "--- testing \"" << v.f_name << "\".\n";
            SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable(v.f_name));
            CATCH_REQUIRE(var != nullptr);
            CATCH_REQUIRE(var->get_name() == v.f_name);
            CATCH_REQUIRE(var->get_type() == "string");
            CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string() == v.f_value);
        }

        // *** INTEGER ***
        struct verify_integer_t
        {
            char const *            f_name = nullptr;
            int                     f_value = 0;
        };

        // verify that the resulting set of variable is indeed sorted
        verify_integer_t integer_verify[] =
        {
            { "i1",   -3 },
            { "i2",   73 },
            { "i3",  506 },
            { "i4", 1003 },
            { "i5", 1004 },
        };

        for(auto const & v : integer_verify)
        {
//std::cerr << "--- testing \"" << v.f_name << "\".\n";
            SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable(v.f_name));
            CATCH_REQUIRE(var != nullptr);
            CATCH_REQUIRE(var->get_name() == v.f_name);
            CATCH_REQUIRE(var->get_type() == "integer");
            CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == v.f_value);
        }

        // *** FLOATING POINT ***
        struct verify_floating_point_t
        {
            char const *            f_name = nullptr;
            double                  f_value = 0.0;
        };

        // verify that the resulting set of variable is indeed sorted
        verify_floating_point_t floating_point_verify[] =
        {
            { "f1", -10.103 },
            { "f2",  -0.3   },
            { "f3",   1.004 },
            { "f4",  50.6   },
            { "f5",  73.5   },
        };

        for(auto const & v : floating_point_verify)
        {
//std::cerr << "--- testing \"" << v.f_name << "\".\n";
            SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable(v.f_name));
            CATCH_REQUIRE(var != nullptr);
            CATCH_REQUIRE(var->get_name() == v.f_name);
            CATCH_REQUIRE(var->get_type() == "floating_point");
            CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_floating_point>(var)->get_floating_point() == v.f_value);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify starting the thread")
    {
        trace tracer(g_verify_starting_thread);

        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_start_thread.rprtr", g_program_start_thread));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());

        // use std::bind() to avoid copies of the tracer object
        //
        s->set_trace_callback(std::bind(&trace::callback, &tracer, std::placeholders::_1, std::placeholders::_2));

        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 12);

        // before we run the script, there are no such variables
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("test"));
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("test_copy_between_dollars");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("runner");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("runner_copy_as_is");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("time_limit");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("time_limit_copy");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("time_from_float");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("host_ip");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("host_ip_copy");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("time_and_host_ip");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("length");
        CATCH_REQUIRE(var == nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        var = s->get_variable("test");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "test");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 33);

        var = s->get_variable("test_copy_between_dollars");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "test_copy_between_dollars");
        CATCH_REQUIRE(var->get_type() == "string");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string() == "$33$");

        var = s->get_variable("runner");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "runner");
        CATCH_REQUIRE(var->get_type() == "floating_point");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_floating_point>(var)->get_floating_point() == 6.07);

        var = s->get_variable("runner_copy_as_is");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "runner_copy_as_is");
        CATCH_REQUIRE(var->get_type() == "string");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string() == "runner = 6.07");

        var = s->get_variable("time_limit");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "time_limit");
        CATCH_REQUIRE(var->get_type() == "timestamp");
        snapdev::timespec_ex const time_limit(1713934141, 107805991);
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_timestamp>(var)->get_timestamp() == time_limit);

        var = s->get_variable("time_limit_copy");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "time_limit_copy");
        CATCH_REQUIRE(var->get_type() == "string");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string() == "limit: 1713934141.107805991");

        var = s->get_variable("host_ip");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "host_ip");
        CATCH_REQUIRE(var->get_type() == "address");
        addr::addr a;
        sockaddr_in ip = {
            .sin_family = AF_INET,
            .sin_port = htons(0),
            .sin_addr = {
                .s_addr = htonl(0x7f070333),
            },
            .sin_zero = {},
        };
        a.set_ipv4(ip);
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_address>(var)->get_address() == a);

        var = s->get_variable("host_ip_copy");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "host_ip_copy");
        CATCH_REQUIRE(var->get_type() == "string");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string() == "Host is at 127.7.3.51 address");

        var = s->get_variable("time_and_host_ip");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "time_and_host_ip");
        CATCH_REQUIRE(var->get_type() == "string");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string() == "time 1713934141.107805991 and address 127.7.3.51...");

        var = s->get_variable("length");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "length");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 51);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify computation (integers)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_integer.rprtr", g_program_verify_computation_integer));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 15);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

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

    CATCH_START_SECTION("reporter_executor: verify computation (floating points)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_floating_point.rprtr", g_program_verify_computation_floating_point));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 63);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

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

    CATCH_START_SECTION("reporter_executor: verify computation (timestamp)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_timestamp.rprtr", g_program_verify_computation_timestamp));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 12);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

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
            { "t31", { 177, 330'000'000 } },
            { "t32", { 158, 310'731'200 } },
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

    CATCH_START_SECTION("reporter_executor: verify hex() function")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_timestamp.rprtr", g_program_verify_hex));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 5);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        struct verify_t
        {
            char const *            f_name = nullptr;
            char const *            f_value = nullptr;
        };

        verify_t verify[] =
        {
            { "t01", "1a4fd2" },
            { "t02", "abcdef" },
            { "t03", "ABCDEF" },
            { "t04", "00000001" },
            { "t05", "00ABCDEF" },
        };

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        for(auto const & v : verify)
        {
            var = s->get_variable(v.f_name);
            CATCH_REQUIRE(var != nullptr);
            CATCH_REQUIRE(var->get_name() == v.f_name);
            CATCH_REQUIRE(var->get_type() == "string");
            CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string() == v.f_value);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify now()")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_now.rprtr", g_program_verify_now));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        var = s->get_variable("about_now");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "about_now");
        CATCH_REQUIRE(var->get_type() == "timestamp");
        snapdev::timespec_ex const value(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_timestamp>(var)->get_timestamp());
        snapdev::timespec_ex const now(snapdev::now());
        snapdev::timespec_ex const lower_value(now - snapdev::timespec_ex(1, 0));
        CATCH_REQUIRE(lower_value <= value);
        CATCH_REQUIRE(now >= value);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify max_pid()")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_max_pid.rprtr", g_program_verify_max_pid));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        var = s->get_variable("top_pid");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "top_pid");
        CATCH_REQUIRE(var->get_type() == "integer");
        std::int64_t const value(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer());
        CATCH_REQUIRE(cppthread::get_pid_max() == value);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify random()")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_random.rprtr", g_program_verify_random));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 4);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        var = s->get_variable("any_number");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "any_number");
        CATCH_REQUIRE(var->get_type() == "integer");

        var = s->get_variable("positive");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "positive");
        CATCH_REQUIRE(var->get_type() == "integer");
        std::int64_t const value(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer());
        CATCH_REQUIRE(value >= 0);

        var = s->get_variable("positive_or_negative");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "positive_or_negative");
        CATCH_REQUIRE(var->get_type() == "integer");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify hostname")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_now.rprtr", g_program_verify_hostname));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        var = s->get_variable("host_name");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "host_name");
        CATCH_REQUIRE(var->get_type() == "string");
        std::string const host_name(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string());
        std::string const expected_name(snapdev::gethostname());
        CATCH_REQUIRE(expected_name == host_name);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify kill with number")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_kill_number.rprtr", g_program_verify_kill_number));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify kill with identifier")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_kill_identifier.rprtr", g_program_verify_kill_identifier));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify kill with string")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_kill_string.rprtr", g_program_verify_kill_string));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify computation (address)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_address.rprtr", g_program_verify_computation_address));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 4);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

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

    CATCH_START_SECTION("reporter_executor: verify computation (concatenation)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_concatenation.rprtr", g_program_verify_computation_concatenation));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 21);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        struct verify_t
        {
            char const *            f_name = nullptr;
            char const *            f_value = nullptr;
            char const *            f_type = nullptr;
        };

        verify_t verify[] =
        {
            { "t01", "identifier", "identifier" },
            { "t11", "single string", "string" },
            { "t12", "single string", "string" },
            { "t13", "single string", "string" },
            { "t14", "double string", "string" },
            { "t21", "identify", "identifier" },
            { "t22", "single string", "string" },
            { "t23", "double string", "string" },
            { "t31", "single36", "string" },
            { "t32", "258single", "string" },
            { "t33", "string102", "string" },
            { "t34", "5005double", "string" },
            { "t41", "single[0-9]+", "regex" },
            { "t42", "[0-9]+single", "regex" },
            { "t43", "string[0-9]+", "regex" },
            { "t44", "[0-9]+double", "regex" },
            { "t45", "a\\|b[0-9]+", "regex" },
            { "t46", "[0-9]+c\\{3,9\\}", "regex" },
            { "t47", "\\[a-z\\]\\?[0-9]+", "regex" },
            { "t48", "[0-9]+a\\?b\\?c\\?", "regex" },
            { "t49", "[0-9]+(a|b|c)?", "regex" },
        };

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var;
        for(auto const & v : verify)
        {
//std::cerr << "--- testing \"" << v.f_name << "\".\n";
            var = s->get_variable(v.f_name);
            CATCH_REQUIRE(var != nullptr);
            CATCH_REQUIRE(var->get_name() == v.f_name);
            CATCH_REQUIRE(var->get_type() == v.f_type);
            CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var)->get_string() == v.f_value);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor: verify computation (string repeat)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_computation_string_repeat.rprtr", g_program_verify_computation_string_repeat));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 4);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

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

    CATCH_START_SECTION("reporter_executor: verify variable in string")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("verify_variable_in_string.rprtr", g_program_verify_variable_in_string));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        struct verify_t
        {
            char const *            f_name = nullptr;
            char const *            f_value = nullptr;
        };

        verify_t verify[] =
        {
            { "foo", "abc" },
            { "bar", "[abc]" },
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

    CATCH_START_SECTION("reporter_executor: print() + message")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("print_message.rprtr", g_program_print_message));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_executor_message", "[executor][reporter][message]")
{
    CATCH_START_SECTION("reporter_executor_message: send/receive one message")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_accept_one_message.rprtr", g_program_accept_one_message));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 16);

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

        CATCH_REQUIRE(e->run());

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == 0);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("command"));
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_string::pointer_t v(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var));
        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->get_string() == "REGISTER");

        var = s->get_variable("register_version");
        CATCH_REQUIRE(var != nullptr);
        v = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var);
        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->get_string() == "1");

        var = s->get_variable("register_service");
        CATCH_REQUIRE(var != nullptr);
        v = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var);
        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->get_string() == "responder");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_message: receive one unwanted/unexpected message")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_receive_unwanted_message.rprtr", g_program_receive_unwanted_message));
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
                , messenger_responder::sequence_t::SEQUENCE_UNWANTED_MESSAGE));
        ed::communicator::instance()->add_connection(messenger);
        e->set_thread_done_callback([messenger]()
            {
                ed::communicator::instance()->remove_connection(messenger);
            });

        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_message: send message with unsupported parameter type fails")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_send_unsupported_message_parameter_type.rprtr", g_program_send_unsupported_message_parameter_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 12);

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

        CATCH_REQUIRE(e->run());
        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: message parameter type \"floating_point\" not supported yet."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_message: save message parameter identifier as an integer fails")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_send_invalid_parameter_value_type.rprtr", g_program_send_invalid_parameter_value_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 12);

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

        CATCH_REQUIRE(e->run());
        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: value \"responder\" not recognized as a valid integer."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_message: save message parameter of type timestamp")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_save_parameter_of_type_timestamp.rprtr", g_program_save_parameter_of_type_timestamp));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        //CATCH_REQUIRE(s->get_statement_size() == 19);

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
                , messenger_responder::sequence_t::SEQUENCE_TIMED_MESSAGE));
        ed::communicator::instance()->add_connection(messenger);
        e->set_thread_done_callback([messenger]()
            {
                ed::communicator::instance()->remove_connection(messenger);
            });

        CATCH_REQUIRE(e->run());
        e->stop();

        CATCH_REQUIRE(s->get_exit_code() == 0);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("register_version"));
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_integer::pointer_t vi(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var));
        CATCH_REQUIRE(vi != nullptr);
        CATCH_REQUIRE(vi->get_type() == "integer");
        CATCH_REQUIRE(vi->get_integer() == 1);

        var = s->get_variable("default_integer");
        CATCH_REQUIRE(var != nullptr);
        vi = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var);
        CATCH_REQUIRE(vi != nullptr);
        CATCH_REQUIRE(vi->get_type() == "integer");
        CATCH_REQUIRE(vi->get_integer() == 0);

        var = s->get_variable("timed_value");
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_timestamp::pointer_t vts(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_timestamp>(var));
        CATCH_REQUIRE(vts != nullptr);
        CATCH_REQUIRE(vts->get_type() == "timestamp");
        snapdev::timespec_ex const param_timestamp(vts->get_timestamp());
        snapdev::timespec_ex const now(snapdev::now());
        snapdev::timespec_ex const minimum_value(now - snapdev::timespec_ex(1, 0));
        CATCH_REQUIRE(param_timestamp >= minimum_value);
        CATCH_REQUIRE(param_timestamp <= now);

        var = s->get_variable("default_time");
        CATCH_REQUIRE(var != nullptr);
        vts = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_timestamp>(var);
        CATCH_REQUIRE(vts != nullptr);
        CATCH_REQUIRE(vts->get_type() == "timestamp");
        CATCH_REQUIRE(vts->get_timestamp() == snapdev::timespec_ex());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_message: save message parameter with unknown type")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_save_parameter_with_unknown_type.rprtr", g_program_save_parameter_with_unknown_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 12);

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

        CATCH_REQUIRE(e->run());
        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: unsupported type \"void\" for save_parameter_value()."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_message: send & receive complete messages")
    {
        // in this case, load the program from a file
        // to verify that this works as expected
        //
        std::string const source_dir(SNAP_CATCH2_NAMESPACE::g_source_dir());
        std::string const filename(source_dir + "/tests/rprtr/send_and_receive_complete_messages");
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(SNAP_CATCH2_NAMESPACE::reporter::create_lexer(filename));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 34);

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

        CATCH_REQUIRE(e->run());

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == 0);

        // we unset that variable, make sure that worked
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("got_register"));
        CATCH_REQUIRE(var == nullptr);

        var = s->get_variable("server");
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_string::pointer_t str(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var));
        CATCH_REQUIRE(str != nullptr);
        CATCH_REQUIRE(str->get_type() == "string");
        CATCH_REQUIRE(str->get_string() == "reporter_test_extension");

        var = s->get_variable("service");
        CATCH_REQUIRE(var != nullptr);
        str = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var);
        CATCH_REQUIRE(str != nullptr);
        CATCH_REQUIRE(str->get_type() == "string");
        CATCH_REQUIRE(str->get_string() == "test_processor");

        var = s->get_variable("sent_server");
        CATCH_REQUIRE(var != nullptr);
        str = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var);
        CATCH_REQUIRE(str != nullptr);
        CATCH_REQUIRE(str->get_type() == "string");
        CATCH_REQUIRE(str->get_string() == "reporter_test");

        var = s->get_variable("sent_service");
        CATCH_REQUIRE(var != nullptr);
        str = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var);
        CATCH_REQUIRE(str != nullptr);
        CATCH_REQUIRE(str->get_type() == "string");
        CATCH_REQUIRE(str->get_string() == "commands_message");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_message: verify last wait (disconnect -> HUP)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_verify_last_wait.rprtr", g_program_last_wait));
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
        CATCH_REQUIRE(e->run());
        e->stop();

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_message: wait for timeout (to make sure we DO NOT receive extra messages)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_wait_for_timeout.rprtr", g_program_wait_for_timeout));
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
        CATCH_REQUIRE(e->run());
        e->stop();

        CATCH_REQUIRE_FALSE(timer->timed_out_prima());
        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_executor_variables", "[executor][reporter][variable]")
{
    CATCH_START_SECTION("reporter_executor_variables: undefined variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_undefined_variable.rprtr", g_program_undefined_variable));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 5);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_variables: detect integer variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_integer_variable.rprtr", g_program_integer_variable));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 10);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_variables: detect string variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_string_variable.rprtr", g_program_string_variable));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 10);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_variables: if variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_if_variable.rprtr", g_program_if_variable));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 104);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_variables: compare and if")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_compare_and_if.rprtr", g_program_compare_and_if));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 116);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE(s->get_exit_code() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_variables: void variable cloning")
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

    CATCH_START_SECTION("reporter_executor_variables: list variable")
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
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: variable_list::add_item() trying to re-add item named \"void_var\"."));

        CATCH_REQUIRE_THROWS_MATCHES(
              l->add_item(var2)
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: variable_list::add_item() trying to re-add item named \"integer_var\"."));

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

    CATCH_START_SECTION("reporter_executor_variables: primary variable references")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("primary_variable_references.rprtr", g_program_primary_variable_references));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 14);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("my_string_var"));
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("longer_string_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("my_integer_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("longer_integer_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("my_floating_point_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("longer_floating_point_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("my_identifier_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("longer_identifier_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("my_regex_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("longer_regex_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("my_address_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("longer_address_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("my_timestamp_var");
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("longer_timestamp_var");
        CATCH_REQUIRE(var == nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        var = s->get_variable("my_string_var");
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_string::pointer_t vs(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var));
        CATCH_REQUIRE(vs != nullptr);
        CATCH_REQUIRE(vs->get_type() == "string");
        CATCH_REQUIRE(vs->get_string() == "foo");

        var = s->get_variable("longer_string_var");
        CATCH_REQUIRE(var != nullptr);
        vs = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var);
        CATCH_REQUIRE(vs != nullptr);
        CATCH_REQUIRE(vs->get_type() == "string");
        CATCH_REQUIRE(vs->get_string() == "foo");

        var = s->get_variable("my_integer_var");
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_integer::pointer_t vi(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var));
        CATCH_REQUIRE(vi != nullptr);
        CATCH_REQUIRE(vi->get_type() == "integer");
        CATCH_REQUIRE(vi->get_integer() == 41);

        var = s->get_variable("longer_integer_var");
        CATCH_REQUIRE(var != nullptr);
        vi = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var);
        CATCH_REQUIRE(vi != nullptr);
        CATCH_REQUIRE(vi->get_type() == "integer");
        CATCH_REQUIRE(vi->get_integer() == 41);

        var = s->get_variable("my_floating_point_var");
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_floating_point::pointer_t vf(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_floating_point>(var));
        CATCH_REQUIRE(vf != nullptr);
        CATCH_REQUIRE(vf->get_type() == "floating_point");
        CATCH_REQUIRE(vf->get_floating_point() == 303.601);

        var = s->get_variable("longer_floating_point_var");
        CATCH_REQUIRE(var != nullptr);
        vf = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_floating_point>(var);
        CATCH_REQUIRE(vf != nullptr);
        CATCH_REQUIRE(vf->get_type() == "floating_point");
        CATCH_REQUIRE(vf->get_floating_point() == 303.601);

        var = s->get_variable("my_identifier_var");
        CATCH_REQUIRE(var != nullptr);
        vs = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var);
        CATCH_REQUIRE(vs != nullptr);
        CATCH_REQUIRE(vs->get_type() == "identifier");
        CATCH_REQUIRE(vs->get_string() == "bar");

        var = s->get_variable("longer_identifier_var");
        CATCH_REQUIRE(var != nullptr);
        vs = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var);
        CATCH_REQUIRE(vs != nullptr);
        CATCH_REQUIRE(vs->get_type() == "identifier");
        CATCH_REQUIRE(vs->get_string() == "bar");

        var = s->get_variable("my_regex_var");
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_regex::pointer_t vre(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_regex>(var));
        CATCH_REQUIRE(vre != nullptr);
        CATCH_REQUIRE(vre->get_type() == "regex");
        CATCH_REQUIRE(vre->get_regex() == "^[regex]$");

        var = s->get_variable("longer_regex_var");
        CATCH_REQUIRE(var != nullptr);
        vre = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_regex>(var);
        CATCH_REQUIRE(vre != nullptr);
        CATCH_REQUIRE(vre->get_type() == "regex");
        CATCH_REQUIRE(vre->get_regex() == "^[regex]$");

        addr::addr a;
        sockaddr_in ip = {
            .sin_family = AF_INET,
            .sin_port = htons(89),
            .sin_addr = {
                .s_addr = htonl(0x0A0C0E10),
            },
            .sin_zero = {},
        };
        a.set_ipv4(ip);

        var = s->get_variable("my_address_var");
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_address::pointer_t va(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_address>(var));
        CATCH_REQUIRE(va != nullptr);
        CATCH_REQUIRE(va->get_type() == "address");
        CATCH_REQUIRE(va->get_address() == a);

        var = s->get_variable("longer_address_var");
        CATCH_REQUIRE(var != nullptr);
        va = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_address>(var);
        CATCH_REQUIRE(va != nullptr);
        CATCH_REQUIRE(va->get_type() == "address");
        CATCH_REQUIRE(va->get_address() == a);

        snapdev::timespec_ex const expected_timestamp{ 1714241733, 419438123 };

        var = s->get_variable("my_timestamp_var");
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_timestamp::pointer_t vts(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_timestamp>(var));
        CATCH_REQUIRE(vts != nullptr);
        CATCH_REQUIRE(vts->get_type() == "timestamp");
        CATCH_REQUIRE(vts->get_timestamp() == expected_timestamp);

        var = s->get_variable("longer_timestamp_var");
        CATCH_REQUIRE(var != nullptr);
        vts = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_timestamp>(var);
        CATCH_REQUIRE(vts != nullptr);
        CATCH_REQUIRE(vts->get_type() == "timestamp");
        CATCH_REQUIRE(vts->get_timestamp() == expected_timestamp);

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_variables: primary variable reference with wrong name")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("primary_variable_reference.rprtr", g_program_wrong_primary_variable_reference));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("my_var"));
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("longer_var");
        CATCH_REQUIRE(var == nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        var = s->get_variable("my_var");
        CATCH_REQUIRE(var != nullptr);
        SNAP_CATCH2_NAMESPACE::reporter::variable_string::pointer_t v(std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var));
        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->get_string() == "foo");

        var = s->get_variable("longer_var");
        CATCH_REQUIRE(var != nullptr);
        v = std::dynamic_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_string>(var);
        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->get_string() == ""); // wrong name so we get an empty string

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_executor_state", "[executor][reporter][error]")
{
    CATCH_START_SECTION("reporter_executor_state: add and read data from the state")
    {
        for(int count(0); count < 10; ++count)
        {
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());

            CATCH_REQUIRE(s->get_server_pid() == getpid());
            CATCH_REQUIRE(s->data_size() == 0);

            SNAP_CATCH2_NAMESPACE::reporter::connection_data_t buf;
            CATCH_REQUIRE(s->read_data(buf, 1024) == -1);

            // clear has no effect here
            //
            s->clear_data();

            CATCH_REQUIRE(s->get_server_pid() == getpid());
            CATCH_REQUIRE(s->data_size() == 0);

            CATCH_REQUIRE(s->read_data(buf, 1024) == -1);

            std::size_t total(0);
            std::vector<std::size_t> sizes(10);
            for(int i(0); i < 10; ++i)
            {
                sizes[i] = rand() % (1024 * 4) + 1;
                total += sizes[i];
            }

            SNAP_CATCH2_NAMESPACE::reporter::connection_data_t data(total);
            for(std::size_t i(0); i < total; ++i)
            {
                data[i] = rand();
            }

            std::size_t offset(0);
            for(int i(0); i < 10; ++i)
            {
                SNAP_CATCH2_NAMESPACE::reporter::connection_data_pointer_t d(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::connection_data_t>(data.data() + offset, data.data() + offset + sizes[i]));
                s->add_data(d);
                offset += sizes[i];
                CATCH_REQUIRE(s->data_size() == static_cast<ssize_t>(offset));
            }
            CATCH_REQUIRE(s->data_size() == static_cast<ssize_t>(total));

            offset = 0;
            while(offset < total)
            {
                std::size_t const expected_size(std::min(64UL, total - offset));
                CATCH_REQUIRE(s->read_data(buf, 64) == static_cast<int>(expected_size));
                CATCH_REQUIRE(buf.size() == expected_size);
                for(std::size_t i(0); i < expected_size; ++i)
                {
                    CATCH_REQUIRE(buf[i] == data[offset + i]);
                }
                offset += expected_size;
            }

            // here the clear has an effect
            //
            s->clear_data();
            CATCH_REQUIRE(s->data_size() == 0);
            CATCH_REQUIRE(s->read_data(buf, 1024) == -1);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_executor_error", "[executor][reporter][error]")
{
    CATCH_START_SECTION("reporter_executor_error: if() before any condition")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("if_too_soon.rprtr", g_program_no_condition));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: trying to use a 'compare' result when none are currently defined."));

        CATCH_REQUIRE_THROWS_MATCHES(
              s->set_compare(SNAP_CATCH2_NAMESPACE::reporter::compare_t::COMPARE_UNDEFINED)
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: 'compare' cannot be set to \"undefined\"."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: compare() with incompatible types")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("compare_with_incompatible_types.rprtr", g_program_compare_with_incompatible_types));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 4);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: compare_with_incompatible_types.rprtr:3: unsupported compare (token types: 3 <=> 39)."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: compare() with non-integer result")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("compare_with_non_integer.rprtr", g_program_compare_with_non_integer));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: compare_with_non_integer.rprtr:1: parameter type mismatch for expression, expected \"integer\", got \"string\" instead."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: compare() with bad positive integer result")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("compare_with_bad_positive_integer.rprtr", g_program_compare_with_bad_positive_integer));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: compare_with_bad_positive_integer.rprtr:1: unsupported integer in compare(), values are limited to -2 to 1."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: compare() with bad negative integer result")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("compare_with_bad_negative_integer.rprtr", g_program_compare_with_bad_negative_integer));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: compare_with_bad_negative_integer.rprtr:1: unsupported integer in compare(), values are limited to -2 to 1."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: kill() with invalid parameter type (timestamp)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("kill_unsupported_timestamp.rprtr", g_program_verify_kill_unsupported_timestamp));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: kill_unsupported_timestamp.rprtr:1: kill(signal: ...) unsupported parameter type."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: kill() with too large an integer")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("kill_integer_too_large.rprtr", g_program_verify_kill_integer_too_large));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: kill_integer_too_large.rprtr:1: kill(signal: ...) unknown signal."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: kill() with an unknown signal name")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("kill_unknown_signal_name.rprtr", g_program_verify_kill_unknown_signal_name));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: kill_unknown_signal_name.rprtr:1: kill(signal: ...) unknown signal."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: exit() + error message")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("exit_error_message.rprtr", g_program_error_message));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();

        CATCH_REQUIRE(s->get_exit_code() == 1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: listen() + listen()")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("two_listen.rprtr", g_program_two_listen));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: the listen() instruction cannot be reused without an intermediate disconnect() instruction."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: label(name: ...) does not accept integers")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("label_bad_type.rprtr", g_program_label_bad_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));

        // label is a special case which we test in the state.cpp way before
        // we reach the executor... (so this is not really an executor test)
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: the value of the \"name\" parameter of the \"label\" statement must be an identifier."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: exit(error_message: ...) does not accept floating points")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("exit_bad_type.rprtr", g_program_exit_bad_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: exit_bad_type.rprtr:1: parameter type mismatch for error_message, expected \"string\", got \"floating_point\" instead."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: verify starting the thread twice")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_start_thread_twice.rprtr", g_program_start_thread_twice));
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
        CATCH_REQUIRE(e->run());
        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: run() instruction found when already running in the background."));

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

    CATCH_START_SECTION("reporter_executor_error: <type> + <type> that are not valid")
    {
        struct bad_additions_t
        {
            char const *    f_code = nullptr;
            SNAP_CATCH2_NAMESPACE::reporter::token_t  f_left_hand_side = SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR;
            SNAP_CATCH2_NAMESPACE::reporter::token_t  f_right_hand_side = SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR;
        };
        constexpr bad_additions_t const bad_additions[] =
        {
            {
                g_program_unsupported_addition_address_address,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
            },
            {
                g_program_unsupported_addition_address_string,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING,
            },
            {
                g_program_unsupported_addition_string_address,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
            },
            {
                g_program_unsupported_addition_address_identifier,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
            },
            {
                g_program_unsupported_addition_identifier_address,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
            },
            {
                g_program_unsupported_addition_identifier_string,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING,
            },
            {
                g_program_unsupported_addition_string_identifier,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
            },
        };

        for(auto const & ba : bad_additions)
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_additions.rprtr", ba.f_code));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "event_dispatcher_exception: unsupported addition (token types: "
                        + std::to_string(static_cast<int>(ba.f_left_hand_side))
                        + " + "
                        + std::to_string(static_cast<int>(ba.f_right_hand_side))
                        + ")."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: <type> - <type> that are not valid")
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
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_subtractions.rprtr", program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "event_dispatcher_exception: unsupported subtraction."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: <type> * <type> that are not valid")
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
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_multiplications.rprtr", program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "event_dispatcher_exception: unsupported multiplication."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: <type> / <type> that are not valid")
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
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_divisions.rprtr", program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "event_dispatcher_exception: unsupported division."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: <type> % <type> that are not valid")
    {
        struct bad_modulo_t
        {
            char const * const                          f_expr = nullptr;
            SNAP_CATCH2_NAMESPACE::reporter::token_t    f_lhs_token = SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR;
            SNAP_CATCH2_NAMESPACE::reporter::token_t    f_rhs_token = SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR;
        };
        constexpr bad_modulo_t const bad_modulos[] =
        {
            {
                g_program_unsupported_modulo_address_address,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
            },
            {
                g_program_unsupported_modulo_address_string,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING,
            },
            {
                g_program_unsupported_modulo_string_address,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
            },
            {
                g_program_unsupported_modulo_address_identifier,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
            },
            {
                g_program_unsupported_modulo_identifier_address,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
            },
            {
                g_program_unsupported_modulo_identifier_string,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING, // the type of string was already converted by this time
            },
            {
                g_program_unsupported_modulo_string_identifier,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
            },
            {
                g_program_unsupported_modulo_string_string,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING, // the type of string was already converted by this time
            },
            {
                g_program_unsupported_modulo_identifier_identifier,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
                SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
            },
        };

        for(auto const & program : bad_modulos)
        {
//std::cerr << "testing [" << program.f_expr << "]\n";
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_modulos.rprtr", program.f_expr));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "event_dispatcher_exception: unsupported modulo (types: "
                        + std::to_string(static_cast<int>(program.f_lhs_token))
                        + " and "
                        + std::to_string(static_cast<int>(program.f_rhs_token))
                        + ")."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: -<types> that are not valid")
    {
        constexpr char const * const bad_negations[] =
        {
            g_program_unsupported_negation_single_string,
            g_program_unsupported_negation_double_string,
            g_program_unsupported_negation_address,
        };

        for(auto const & program : bad_negations)
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_negate.rprtr", program));
            SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
            SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
            p->parse_program();

            CATCH_REQUIRE(s->get_statement_size() == 1);

            SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->start()
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "event_dispatcher_exception: unsupported negation."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: variable reference without a '}'")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_negate.rprtr", g_program_unterminated_double_string_variable));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: invalid_negate.rprtr:2: found unclosed variable in \"ref. ${my_var\"."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: regex variable in double string")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_negate.rprtr", g_program_regex_in_double_string_variable));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: found variable of type \"regex\" which is not yet supported in ${...}."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: variable reference without a name")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_negate.rprtr", g_program_double_string_variable_without_name));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: invalid_negate.rprtr:1: found variable without a name in \"ref. ${} is empty\"."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: <string> * <negative> is not valid")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_string_multiplication_negative.rprtr", g_program_unsupported_negation_repeat));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: string repeat needs to be positive and under 1001."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: <string> * <large repeat> is not valid")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("invalid_string_multiplication_large.rprtr", g_program_unsupported_large_repeat));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: string repeat needs to be positive and under 1001."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: exit() with timeout & error_message is invalid")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("bad_exit.rprtr", g_program_bad_exit));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: bad_exit.rprtr:1: \"timeout\" and \"error_message\" from the exit() instruction are mutually exclusive."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: exit() with timeout which is not a number")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("bad_exit.rprtr", g_program_bad_exit_timeout));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: bad_exit.rprtr:1: parameter type mismatch for timeout, expected \"number\", got \"string\" instead."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: exit() with timeout which is not a number")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("bad_print.rprtr", g_program_bad_print_message));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: bad_print.rprtr:1: parameter type mismatch for message, expected \"string\", got \"identifier\" instead."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: send_message() when not connected")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("bad_send_message.rprtr", g_program_send_message_without_connection));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: send_message() has no connection to send a message to."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: if(variable) with invalid type")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("if_invalid_type.rprtr", g_program_if_invalid_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 5);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: if(variable: ...) only supports variables of type integer or floating point."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: wait() before starting thread")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("wait_outside_thread.rprtr", g_program_wait_outside_thread));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: wait() used before run()."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: wait() with invalid mode")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_wait_invalid_mode.rprtr", g_program_wait_invalid_mode));
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

        CATCH_REQUIRE(e->run());

        // the thread exception happens when e->stop() is called
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: program_wait_invalid_mode.rprtr:2: unknown mode \"not_this_one\" in wait()."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: wait() + drain without connections")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_wait_no_connection.rprtr", g_program_wait_no_connections));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        CATCH_REQUIRE(e->run());

        // the thread exception happens when e->stop() is called
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: no connections to wait() on."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: try reading missing file")
    {
        std::string const source_dir(SNAP_CATCH2_NAMESPACE::g_source_dir());
        std::string const filename(source_dir + "/tests/rprtr/not_this_one");
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(SNAP_CATCH2_NAMESPACE::reporter::create_lexer(filename));
        CATCH_REQUIRE(l == nullptr);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: verify that the executor::run() function does a try/catch as expected")
    {
        // in this case, load the program from a file
        // to verify that this works as expected
        //
        std::string const source_dir(SNAP_CATCH2_NAMESPACE::g_source_dir());
        std::string const filename(source_dir + "/tests/rprtr/send_and_receive_complete_messages");
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(SNAP_CATCH2_NAMESPACE::reporter::create_lexer(filename));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 34);

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
                , messenger_responder::sequence_t::SEQUENCE_READY_THROW
                , ed::DEFAULT_PAUSE_BEFORE_RECONNECTING));
        ed::communicator::instance()->add_connection(messenger);
        messenger_timer::pointer_t timer(std::make_shared<messenger_timer>(messenger));
        ed::communicator::instance()->add_connection(timer);
        messenger->set_timer(timer);

        // the exception capture in run() is not returned; it should be
        // printed in the console, making it possible to see what happened
        //
        CATCH_REQUIRE_FALSE(e->run());

        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: ppoll() timed out."));

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == -1);

        // in this case, the variable does not get unset because the
        // crash happens before we have the chance to do that
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("got_register"));
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: verify that the executor::run() function does a try/catch of non-standard exceptions")
    {
        // in this case, load the program from a file
        // to verify that this works as expected
        //
        std::string const source_dir(SNAP_CATCH2_NAMESPACE::g_source_dir());
        std::string const filename(source_dir + "/tests/rprtr/send_and_receive_complete_messages");
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(SNAP_CATCH2_NAMESPACE::reporter::create_lexer(filename));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 34);

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
                , messenger_responder::sequence_t::SEQUENCE_READY_THROW_WHAT));
        ed::communicator::instance()->add_connection(messenger);
        messenger_timer::pointer_t timer(std::make_shared<messenger_timer>(messenger));
        ed::communicator::instance()->add_connection(timer);
        messenger->set_timer(timer);

        // the exception capture in run() is not returned; it should be
        // printed in the console, making it possible to see what happened
        //
        CATCH_REQUIRE_FALSE(e->run());

        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: ppoll() timed out."));

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == -1);

        // in this case, the variable does not get unset because the
        // crash happens before we have the chance to do that
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("got_register"));
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: verify that the run() instruction does throw")
    {
        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("run"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());

        CATCH_REQUIRE_THROWS_MATCHES(
              inst->func(*s)
            , ed::implementation_error
            , Catch::Matchers::ExceptionMessage("implementation_error: run::func() was called when it should be intercepted by the executor."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: invalid timestamp for set_variable() cast")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_invalid_string_to_timestamp_cast.rprtr", g_program_invalid_string_to_timestamp_cast));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));

        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: invalid timestamp, a valid floating point was expected (1713b34141.10780g991)."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: unknown string cast")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_unknown_string_cast.rprtr", g_program_unknown_string_cast));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));

        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: casting from \"string\" to \"unknown\" is not yet implemented."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: unknown timestamp cast")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_unknown_string_cast.rprtr", g_program_unknown_timestamp_cast));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));

        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: casting from \"timestamp\" to \"unknown\" is not yet implemented."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: unknown timestamp cast")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_unknown_timestamp_cast.rprtr", g_program_unknown_source_cast));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));

        // this test will fail once we implement such; at some point, all the different types will be supported and we'll have to remove this test...
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: casting from \"address\" to \"string\" is not yet implemented."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: sort() var1 missing")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_sort_var1_missing.rprtr", g_program_sort_var1_missing));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));

        // this fails way before the inst_sort.func() gets called
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: parameter \"var1\" is required by \"sort\"."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: sort() var1 name must be a string")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_sort_var1_name_not_string.rprtr", g_program_sort_var1_not_string));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));

        // this fails before tje inst_sort.func() gets called
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: program_sort_var1_name_not_string.rprtr:1: parameter type mismatch for var1, expected \"string_or_identifier\", got \"integer\" instead."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: sort() var1 does not name an existing variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_sort_var1_not_found.rprtr", g_program_sort_var1_not_found));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));

        // this fails before tje inst_sort.func() gets called
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: program_sort_var1_not_found.rprtr:1: variable named \"not_defined\" not found."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: sort() does not accept all types yet")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_sort_wrong_type.rprtr", g_program_sort_wrong_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 2);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));

        // this fails before tje inst_sort.func() gets called
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: program_sort_wrong_type.rprtr:2:"
                " sort only supports strings, integers, or floating points."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: sort() does not accept mixed types")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_sort_mixed_types.rprtr", g_program_sort_mixed_types));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 4);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));

        // this fails before tje inst_sort.func() gets called
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: program_sort_mixed_types.rprtr:4:"
                " sort only supports one type of data (\"string\" in this case) for all the specified variables. \"integer\" is not compatible."));

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error: listen() with unknown connection type")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("listen_with_unknown_connection_type.rprtr", g_program_listen_with_unknown_connection_type));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 1);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        CATCH_REQUIRE_THROWS_MATCHES(
              e->start()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: unknown type \"unknown\" for listen()."));
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_executor_error_message", "[executor][reporter][error]")
{
    CATCH_START_SECTION("reporter_executor_error_message: verify message fails")
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
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message expected \"sent_server\", set to \"\", to match \"not_this_one\".",
            },
            {
                g_program_verify_message_fail_sent_service,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message expected \"sent_service\", set to \"\", to match \"not_this_one\".",
            },
            {
                g_program_verify_message_fail_server,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message expected \"server\", set to \"\", to match \"not_this_one\".",
            },
            {
                g_program_verify_message_fail_service,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message expected \"service\", set to \"\", to match \"not_this_one\".",
            },
            {
                g_program_verify_message_fail_command,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message expected \"command\", set to \"REGISTER\", to match \"NOT_THIS_ONE\".",
            },
            {
                g_program_verify_message_fail_forbidden,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message forbidden parameter \"version\" was found in this message.",
            },
            {
                g_program_verify_message_fail_required,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message required parameter \"not_this_one\" was not found in this message.",
            },
            {
                g_program_verify_message_fail_required_int_value,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message expected parameter \"version\" to be an integer set to \"200\" but found \"1\" instead.",
            },
            {
                g_program_verify_message_fail_required_str_value,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message expected parameter \"service\" to be a string set to \"not_this_one\" but found \"responder\" instead.",
            },
            {
                g_program_verify_message_fail_required_long_str_value,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message expected parameter \"service\" to be a string set to \"...responderresponderresponderresponderresponderresponderresponderresponderresponderresponderresponderresponderresponderresponder\" but found \"...\" instead.",
            },
            {
                g_program_verify_message_fail_required_flt_value,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message parameter type \"floating_point\" not supported yet.",
            },
            {
                g_program_verify_message_fail_required_timestamp_value,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message expected parameter \"version\", set to \"Thu Jan  1 00:00:01.000000000 1970\", to match timestamp \"Thu Jan  1 00:02:03.000000000 1970\".",
            },
            {
                g_program_verify_message_fail_timestamp_command,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message value \"command\" does not support type \"timestamp\".",
            },
            {
                g_program_verify_message_fail_unexpected_command,
                "event_dispatcher_exception: program_verify_message_fail.rprtr:9: message expected \"command\", set to \"REGISTER\", to match regex \"^NOT_THIS_ONE$\".",
            },
        };
        for(auto const & bv : bad_verifications)
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_verify_message_fail.rprtr", bv.f_program));
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

            CATCH_REQUIRE(e->run());

            // the thread exception happens when e->stop() is called
            //
            CATCH_REQUIRE_THROWS_MATCHES(
                  e->stop()
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage(bv.f_error));

            // if we exited because of our timer, then the test did not pass
            //
            CATCH_REQUIRE_FALSE(timer->timed_out_prima());

            CATCH_REQUIRE(s->get_exit_code() == -1);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error_message: wait for nothing (should time out)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_wait_for_nothing.rprtr", g_program_wait_for_nothing));
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
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: ppoll() timed out."));

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_executor_error_message: check parameter with incorrect regex fails")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_regex_parameter_no_match.rprtr", g_program_regex_parameter_no_match));
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
        CATCH_REQUIRE(e->run());

        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: program_regex_parameter_no_match.rprtr:9: message expected parameter \"version\", set to \"1\", to match regex \"_[a-z]+\"."));

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());

        CATCH_REQUIRE(s->get_exit_code() == -1);
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
