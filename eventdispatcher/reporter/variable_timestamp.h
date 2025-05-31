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
#pragma once

// self
//
#include    <eventdispatcher/reporter/variable.h>


// snapdev
//
#include    <snapdev/timespec_ex.h>



// view these as an extension of the snapcatch2 library
namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



class variable_timestamp
    : public variable
{
public:
    typedef std::shared_ptr<variable_timestamp>        pointer_t;

                            variable_timestamp(std::string const & name);

    snapdev::timespec_ex    get_timestamp() const;
    void                    set_timestamp(snapdev::timespec_ex i);

    // variable implementation
    //
    virtual variable::pointer_t
                            clone(std::string const & name) const override;

private:
    snapdev::timespec_ex    f_timestamp = snapdev::timespec_ex();
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
