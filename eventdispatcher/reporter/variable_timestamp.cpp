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
#include    "variable_timestamp.h"



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



variable_timestamp::variable_timestamp(std::string const & name)
    : variable(name, "timestamp")
{
}


snapdev::timespec_ex variable_timestamp::get_timestamp() const
{
    return f_timestamp;
}


void variable_timestamp::set_timestamp(snapdev::timespec_ex timestamp)
{
    f_timestamp = timestamp;
}


variable::pointer_t variable_timestamp::clone(std::string const & name) const
{
    pointer_t clone(std::make_shared<variable_timestamp>(name));
    clone->f_timestamp = f_timestamp;
    return clone;
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
