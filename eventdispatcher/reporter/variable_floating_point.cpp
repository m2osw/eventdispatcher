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

// self
//
#include    "variable_floating_point.h"


namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



variable_floating_point::variable_floating_point(std::string const & name)
    : variable(name, "floating_point")
{
}


double variable_floating_point::get_floating_point() const
{
    return f_floating_point;
}


void variable_floating_point::set_floating_point(double d)
{
    f_floating_point = d;
}


variable::pointer_t variable_floating_point::clone(std::string const & name) const
{
    pointer_t clone(std::make_shared<variable_floating_point>(name));
    clone->f_floating_point = f_floating_point;
    return clone;
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
