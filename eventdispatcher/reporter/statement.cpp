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


namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



statement::statement(instruction::pointer_t inst)
    : f_instruction(inst)
{
}


instruction::pointer_t statement::get_instruction() const
{
    return f_instruction;
}


void statement::add_parameter(variable::pointer_t var)
{
    f_parameters[var->get_name()] = var;
}


variable::pointer_t statement::get_parameter(std::string const & name) const
{
    auto it(f_parameters.find(name));
    if(it == f_parameters.end())
    {
        return variable::pointer_t();
    }
    return it->second;
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
