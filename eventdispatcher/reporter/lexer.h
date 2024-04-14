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
#include    "token.h"


// libutf8
//
#include    <libutf8/libutf8.h>
#include    <libutf8/iterator.h>


// C++
//
#include    <memory>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



class lexer
{
public:
    typedef std::shared_ptr<lexer>  pointer_t;

                            lexer(
                                  std::string const & filename
                                , std::string const & program);

    token                   next_token();
    void                    error(token & t, std::string const & msg);

private:
    char32_t                getc();
    void                    ungetc(char32_t c);

    std::string             f_filename = std::string();
    std::string             f_program = std::string();
    libutf8::utf8_iterator  f_iterator;
    char32_t                f_unget[16];
    std::size_t             f_unget_pos = 0;
    std::uint32_t           f_line = 1;
    std::uint32_t           f_last_line = 0;
    std::uint32_t           f_column = 1;
    std::uint32_t           f_last_column = 0;
    double                  f_number = 0.0;
    std::string             f_string = std::string();
    std::string             f_error = std::string();
};


lexer::pointer_t create_lexer(std::string const & filename);



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
