// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
#include    <eventdispatcher/reporter/expression.h>
#include    <eventdispatcher/reporter/instruction.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



class statement
{
public:
    typedef std::shared_ptr<statement>  pointer_t;
    typedef std::vector<pointer_t>      vector_t;

                        statement(instruction::pointer_t inst);

    void                set_filename(std::string const & filename);
    std::string const & get_filename() const;
    void                set_line(std::uint32_t line);
    std::uint32_t       get_line() const;
    std::string const & get_location() const;

    instruction::pointer_t
                        get_instruction() const;

    void                add_parameter(std::string const & name, expression::pointer_t expr);
    expression::pointer_t
                        get_parameter(std::string const & name) const;
    void                verify_parameters() const;

private:
    instruction::pointer_t
                        f_instruction = nullptr;
    std::string         f_filename = std::string(); // TODO: look into a way to avoid repetition
    std::uint32_t       f_line = 0;
    mutable std::string f_location = std::string();
    expression::map_t   f_parameters = expression::map_t();
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
