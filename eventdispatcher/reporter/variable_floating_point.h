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

// self
//
#include    <eventdispatcher/reporter/variable.h>


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



class variable_floating_point
    : public variable
{
public:
    typedef std::shared_ptr<variable_floating_point>        pointer_t;

                            variable_floating_point(std::string const & name);

    double                  get_floating_point() const;
    void                    set_floating_point(double i);

private:
    double                  f_floating_point = 0.0;
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
