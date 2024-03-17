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
#include    "token.h"


// C++
//
#include    <stdexcept>


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



token_t token::get_token() const
{
    return f_token;
}


void token::set_token(token_t t)
{
    if(f_token != token_t::TOKEN_EOF
    && t != token_t::TOKEN_ERROR)
    {
        throw std::logic_error("trying to modify token type to something other than an error.");
    }

    f_token = t;
}


std::uint32_t token::get_line() const
{
    return f_line;
}


void token::set_line(std::uint32_t line)
{
    if(f_line != 0)
    {
        throw std::logic_error("trying to modify line number, not allowed anymore.");
    }

    f_line = line;
}


std::uint32_t token::get_column() const
{
    return f_column;
}


void token::set_column(std::uint32_t column)
{
    if(f_column != 0)
    {
        throw std::logic_error("trying to modify column number, not allowed anymore.");
    }

    f_column = column;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
__int128 token::get_integer() const
{
    return f_integer;
}
#pragma GCC diagnostic pop


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
void token::set_integer(__int128 value)
{
    f_integer = value;
}
#pragma GCC diagnostic pop


double token::get_floating_point() const
{
    return f_floating_point;
}


void token::set_floating_point(double value)
{
    f_floating_point = value;
}


std::string const & token::get_string() const
{
    return f_string;
}


void token::set_string(std::string const & value)
{
    f_string = value;
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
