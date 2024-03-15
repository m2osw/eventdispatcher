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



// view these as an extension of the snapcatch2 library
namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



typedef std::uint32_t       ip_t;


class state
{
public:
    ip_t                    get_ip() const;
    void                    set_ip(ip_t ip);

    statement::pointer_t    get_statement() const;
    void                    set_statement(statement::pointer_t stmt);

    void                    clear_parameter();
    void                    add_parameter(variable::pointer_t param);
    variable::pointer_t     get_parameter(std::string const & name) const;

    variable::pointer_t     get_variable(std::string const & name) const;
    void                    set_variable(variable::pointer_t variable);

    void                    execute_instruction();

private:
    ip_t                    f_ip = 0;                               // instruction pointer in f_program
    statement::vector_t     f_program = statement::vector_t();      // instruction being executed with original parameters
    variable::map_t         f_parameters = variable::map_t();       // parameters at time of call (replaced variables in strings, etc.)
    variable::map_t         f_variables = variable::map_t();        // program variables ("globals")
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
