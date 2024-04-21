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
#include    "variable_address.h"



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



variable_address::variable_address(std::string const & name)
    : variable(name, "address")
{
}


addr::addr variable_address::get_address() const
{
    return f_address;
}


void variable_address::set_address(addr::addr const & a)
{
    f_address = a;
}


variable::pointer_t variable_address::clone(std::string const & name) const
{
    pointer_t clone(std::make_shared<variable_address>(name));
    clone->f_address = f_address;
    return clone;
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
