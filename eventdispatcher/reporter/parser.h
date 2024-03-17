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
#include    "lexer.h"
#include    "statement.h"


// C++
//
//#include    <vector>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



class parser
{
public:
    typedef std::shared_ptr<parser>  pointer_t;

                            parser(lexer::pointer_t l);

    statement::vector_t     parse_program();

private:
    bool                    next_token();
    void                    one_statement();
    void                    parameters();
    void                    one_parameter();
    expression::pointer_t   expression_list();
    expression::pointer_t   list_item();
    expression::pointer_t   additive();
    expression::pointer_t   multiplicative();
    expression::pointer_t   primary();

    lexer::pointer_t        f_lexer = lexer::pointer_t();
    token                   f_token = token();
    statement::vector_t     f_statements = statement::vector_t();
    statement::pointer_t    f_statement = statement::pointer_t();
    variable::pointer_t     f_parameter = variable::pointer_t();
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
