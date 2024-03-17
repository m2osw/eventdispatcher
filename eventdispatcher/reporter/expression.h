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
#include    <eventdispatcher/reporter/token.h>


// C++
//
#include    <map>
#include    <memory>
#include    <vector>



// view these as an extension of the snapcatch2 library
namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



enum class operator_t
{
    OPERATOR_NULL,
    OPERATOR_NAMED,                 // IDENTIFIER [ ':' expression ] (for lists of values)
    OPERATOR_LIST,                  // { ..., ..., ... }
    OPERATOR_PRIMARY,               // a primary value in f_token
    OPERATOR_IDENTITY,              // + <additive>
    OPERATOR_NEGATE,                // - <additive>
    OPERATOR_ADD,
    OPERATOR_SUBTRACT,
    OPERATOR_MULTIPLY,
    OPERATOR_DIVIDE,
    OPERATOR_MODULO,
};


/** \brief An expression in the parameter list.
 *
 * We support very basic expressions in our list of parameters. The
 * expressions remain in the form of a tree which we can execute
 * at the time the instruction is called so that way we can make use
 * of dynamic values.
 */
class expression
{
public:
    typedef std::shared_ptr<expression>         pointer_t;
    typedef std::vector<pointer_t>              vector_t;
    typedef std::map<std::string, pointer_t>    map_t;

    operator_t                  get_operator() const;
    void                        set_operator(operator_t expr);
    std::size_t                 get_expression_size() const;
    pointer_t                   get_expression(int idx) const;
    void                        add_expression(pointer_t expr);
    token const &               get_token() const;
    void                        set_token(token const & t);
    //variable::pointer_t         get_variable() const;
    //void                        set_variable(variable::pointer_t var);

private:
    operator_t                  f_operator = operator_t::OPERATOR_NULL;
    vector_t                    f_expressions = vector_t();
    token                       f_token = token();
    //variable::pointer_t         f_variable = variable::pointer_t();
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
