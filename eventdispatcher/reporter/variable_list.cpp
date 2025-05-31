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
#include    "variable_list.h"



// eventdispatcher
//
#include    <eventdispatcher/exception.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



variable_list::variable_list(std::string const & name)
    : variable(name, "list")
{
}


std::size_t variable_list::get_item_size() const
{
    return f_items.size();
}


variable::pointer_t variable_list::get_item(int idx) const
{
    if(static_cast<std::size_t>(idx) >= f_items.size())
    {
        return variable::pointer_t();
    }

    auto it(f_items.begin());
    for(int i(0); i < idx; ++i, ++it);

    return it->second;
}


variable::pointer_t variable_list::get_item(std::string const & name) const
{
    auto it(f_items.find(name));
    if(it == f_items.end())
    {
        return variable::pointer_t();
    }

    return it->second;
}


void variable_list::add_item(variable::pointer_t v)
{
    if(f_items.find(v->get_name()) != f_items.end())
    {
        throw ed::runtime_error(
              "variable_list::add_item() trying to re-add item named \""
            + v->get_name()
            + "\".");
    }

    f_items[v->get_name()] = v;
}


variable::pointer_t variable_list::clone(std::string const & name) const
{
    pointer_t clone(std::make_shared<variable_list>(name));
    for(auto const & item : f_items)
    {
        clone->add_item(item.second->clone(item.first));
    }
    return clone;
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
