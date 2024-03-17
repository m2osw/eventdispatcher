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
#include    <string>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



enum class token_t : char32_t
{
    TOKEN_EOF               = static_cast<char32_t>(-1),
    TOKEN_ERROR             = 0,                // an error occurred, exit immediately

    TOKEN_IDENTIFIER,
    TOKEN_FLOATING_POINT,
    TOKEN_INTEGER,
    TOKEN_TIMESPEC,             // @<timestamp> or @"date time"
    TOKEN_ADDRESS,              // <ipv4> or <[ipv6]>
    TOKEN_VARIABLE,             // $<name> or ${<name>}

    TOKEN_OPEN_PARENTHESIS  = '(',
    TOKEN_CLOSE_PARENTHESIS = ')',
    TOKEN_OPEN_CURLY_BRACE  = '{',
    TOKEN_CLOSE_CURLY_BRACE = '}',
    TOKEN_COMMA             = ',',
    TOKEN_COLON             = ':',
    TOKEN_EQUAL             = '=',  // for parameters: <name>=<value>
    TOKEN_DOUBLE_STRING     = '"',
    TOKEN_SINGLE_STRING     = '\'',
    TOKEN_PLUS              = '+',
    TOKEN_MINUS             = '-',
    TOKEN_MULTIPLY          = '*',
    TOKEN_DIVIDE            = '/',
    TOKEN_MODULO            = '%',
};


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
class token
{
public:
    token_t                 get_token() const;
    void                    set_token(token_t t);
    std::uint32_t           get_line() const;
    void                    set_line(std::uint32_t line);
    std::uint32_t           get_column() const;
    void                    set_column(std::uint32_t column);

    __int128                get_integer() const;
    void                    set_integer(__int128 value);
    double                  get_floating_point() const;
    void                    set_floating_point(double value);
    std::string const &     get_string() const;
    void                    set_string(std::string const & value);

private:
    token_t                 f_token = token_t::TOKEN_EOF;
    std::uint32_t           f_line = 0;
    std::uint32_t           f_column = 0;
    __int128                f_integer = 0;
    double                  f_floating_point = 0.0;
    std::string             f_string = std::string();
};
#pragma GCC diagnostic pop



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
