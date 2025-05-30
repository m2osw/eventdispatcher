// Copyright (c) 2025  Made to Order Software Corp.  All Rights Reserved
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
#include    "variable_array.h"



// eventdispatcher
//
#include    <eventdispatcher/exception.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



variable_array::variable_array(std::string const & name)
    : variable(name, "array")
{
}


std::size_t variable_array::get_item_size() const
{
    return f_items.size();
}


variable::pointer_t variable_array::get_item(int idx) const
{
    if(static_cast<std::size_t>(idx) >= f_items.size())
    {
        return variable::pointer_t();
    }

    return f_items[idx];
}


void variable_array::add_item(variable::pointer_t v)
{
    f_items.push_back(v);
}


variable::pointer_t variable_array::clone(std::string const & name) const
{
    pointer_t clone(std::make_shared<variable_array>(name));
    for(auto const & item : f_items)
    {
        clone->add_item(item->clone(item->get_name()));
    }
    return clone;
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
