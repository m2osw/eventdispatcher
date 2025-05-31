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



class state;


struct parameter_declaration
{
    char const *    f_name = nullptr;
    char const *    f_type = nullptr;
    bool            f_required = true;
};


class instruction
{
public:
    typedef std::shared_ptr<instruction>        pointer_t;
    typedef std::map<std::string, pointer_t>    map_t;

                            instruction(std::string const & name);
    virtual                 ~instruction();

    std::string const &     get_name() const;

    virtual void            func(state & s) = 0;
    virtual parameter_declaration const *
                            parameter_declarations() const;

private:
    std::string             f_name = std::string();
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
