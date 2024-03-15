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
#include    "state.h"


namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



ip_t state::get_ip() const
{
    return f_ip;
}


void state::set_ip(ip_t ip)
{
    if(ip >= f_program.size())
    {
        throw std::out_of_range("ip out of program not allowed.");
    }

    f_ip = ip;
}


variable::pointer_t state::get_variable(std::string const & name) const
{
    auto it(f_variables.find(name));
    if(it == f_variables.end())
    {
        return variable::pointer_t();
    }
    return it->second;
}


void state::set_variable(variable::pointer_t var)
{
    f_variables[var->get_name()] = var;
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
