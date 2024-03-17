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
#include    "expression.h"


namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



operator_t expression::get_operator() const
{
    return f_operator;
}


void expression::set_operator(operator_t op)
{
    f_operator = op;
}


std::size_t expression::get_expression_size() const
{
    return f_expressions.size();
}


expression::pointer_t expression::get_expression(int idx) const
{
    if(static_cast<std::size_t>(idx) >= f_expressions.size())
    {
        throw std::overflow_error("index too large to get sub-expression.");
    }
    return f_expressions[idx];
}


void expression::add_expression(pointer_t expr)
{
    f_expressions.push_back(expr);
}


token const & expression::get_token() const
{
    return f_token;
}


void expression::set_token(token const & t)
{
    f_token = t;
}


//variable::pointer_t expression::get_variable() const
//{
//    return f_variable;
//}
//
//
//void expression::set_variable(variable::pointer_t var)
//{
//    f_variable = var;
//}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
