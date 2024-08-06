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

// self
//
#include    "statement.h"



// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



statement::statement(instruction::pointer_t inst)
    : f_instruction(inst)
{
    if(f_instruction == nullptr)
    {
        throw std::logic_error("an instruction must always be attached to a statement.");
    }
}


void statement::set_filename(std::string const & filename)
{
    f_filename = filename;
}


std::string const & statement::get_filename() const
{
    return f_filename;
}


void statement::set_line(std::uint32_t line)
{
    f_line = line;
}


std::uint32_t statement::get_line() const
{
    return f_line;
}


std::string const & statement::get_location() const
{
    if(f_location.empty())
    {
        f_location = f_filename;
        f_location += ':';
        f_location += std::to_string(f_line);
        f_location += ": ";
    }

    return f_location;
}


instruction::pointer_t statement::statement::get_instruction() const
{
    return f_instruction;
}


void statement::add_parameter(std::string const & name, expression::pointer_t expr)
{
    auto const it(f_parameters.find(name));
    if(it != f_parameters.end())
    {
        throw std::runtime_error("parameter \"" + name + "\" defined more than once.");
    }

    for(parameter_declaration const * decl(f_instruction->parameter_declarations());
        decl->f_name != nullptr;
        ++decl)
    {
        if(name == decl->f_name)
        {
            f_parameters[name] = expr;
            return;
        }
    }

    throw std::runtime_error(
              "parameter \""
            + name
            + "\" not accepted by \""
            + f_instruction->get_name()
            + "\".");
}


expression::pointer_t statement::get_parameter(std::string const & name) const
{
    auto it(f_parameters.find(name));
    if(it == f_parameters.end())
    {
        return expression::pointer_t();
    }
    return it->second;
}


void statement::verify_parameters() const
{
    for(parameter_declaration const * decl(f_instruction->parameter_declarations());
        decl->f_name != nullptr;
        ++decl)
    {
        if(decl->f_required)
        {
            auto const it(f_parameters.find(decl->f_name));
            if(it == f_parameters.end())
            {
                throw std::runtime_error(
                          "parameter \""
                        + std::string(decl->f_name)
                        + "\" is required by \""
                        + f_instruction->get_name()
                        + "\".");
            }
        }
    }
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
