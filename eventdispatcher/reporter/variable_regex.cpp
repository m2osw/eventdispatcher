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
#include    "variable_regex.h"



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



variable_regex::variable_regex(std::string const & name)
    : variable(name, "regex")
{
}


std::string const & variable_regex::get_regex() const
{
    return f_regex;
}


void variable_regex::set_regex(std::string const & regex)
{
    f_regex = regex;
}


variable::pointer_t variable_regex::clone(std::string const & name) const
{
    pointer_t clone(std::make_shared<variable_regex>(name));
    clone->f_regex = f_regex;
    return clone;
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
