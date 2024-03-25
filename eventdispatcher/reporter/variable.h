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
#pragma once

// catch2
//
#include    <catch2/snapcatch2.hpp>


// C++
//
#include    <map>
#include    <memory>
#include    <string>



// view these as an extension of the snapcatch2 library
namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



/** \brief A variable in the reporter's environment.
 *
 * Our variables are variants. You overload the base class to add support
 * for your own type.
 *
 * \code
 *     class var_message
 *         : public variable
 *     {
 *     ...
 *         void set_value(message const & msg) { f_value = msg; }
 *         message const & get_value() const { return f_value(); }
 *     ...
 *     };
 * \endcode
 */
class variable
{
public:
    typedef std::shared_ptr<variable>           pointer_t;
    typedef std::map<std::string, pointer_t>    map_t;

                            variable(
                                  std::string const & name
                                , std::string const & type);
    virtual                 ~variable();

    std::string const &     get_name() const;
    std::string const &     get_type() const;

    virtual pointer_t       clone(std::string const & name) const = 0;

private:
    std::string             f_name = std::string();
    std::string             f_type = std::string();
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
