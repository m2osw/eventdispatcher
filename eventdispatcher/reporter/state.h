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
#pragma once

// self
//
#include    <eventdispatcher/reporter/instruction.h>
#include    <eventdispatcher/reporter/statement.h>
#include    <eventdispatcher/reporter/variable.h>


// eventdispatcher
//
#include    <eventdispatcher/message.h>



// view these as an extension of the snapcatch2 library
namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



typedef std::uint32_t       ip_t;

enum class compare_t : int8_t
{
    COMPARE_UNDEFINED = -3,
    COMPARE_UNORDERED = -2,
    COMPARE_LESS      = -1,
    COMPARE_EQUAL     = 0,
    COMPARE_GREATER   = 1,
};


class state
{
public:
    typedef std::shared_ptr<state>  pointer_t;

    ip_t                    get_ip() const;
    void                    set_ip(ip_t ip);
    void                    push_ip();
    void                    pop_ip();

    statement::pointer_t    get_statement(ip_t ip) const;
    std::size_t             get_statement_size() const;
    void                    add_statement(statement::pointer_t stmt);

    void                    clear_parameters();
    void                    add_parameter(variable::pointer_t param);
    variable::pointer_t     get_parameter(std::string const & name, bool required = false) const;

    variable::pointer_t     get_variable(std::string const & name) const;
    void                    set_variable(variable::pointer_t variable);

    ip_t                    get_label_position(std::string const & name) const;

    void                    execute_instruction();

    compare_t               get_compare() const;
    void                    set_compare(compare_t c);

    ed::message const &     get_message() const;
    void                    set_message(ed::message const & msg);

    int                     get_exit_code() const;
    void                    set_exit_code(int code);

private:
    typedef std::map<std::string, ip_t>     label_map_t;
    typedef std::vector<ip_t>               call_statck_t;

    ip_t                    f_ip = 0;                               // instruction pointer in f_program
    call_statck_t           f_stack = call_statck_t();
    statement::vector_t     f_program = statement::vector_t();      // instruction being executed with original parameters
    variable::map_t         f_parameters = variable::map_t();       // parameters at time of call (replaced variables in strings, etc.)
    variable::map_t         f_variables = variable::map_t();        // program variables ("globals")
    label_map_t             f_labels = label_map_t();
    compare_t               f_compare = compare_t::COMPARE_UNDEFINED;
    int                     f_exit_code = -1;
    ed::message             f_message = ed::message();
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
