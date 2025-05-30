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

// this diagnostic has to be turned off "globally" so the catch2 does not
// generate the warning on the floating point == operator
//

// self
//
#include    "catch_main.h"


// reporter
//
#include    <eventdispatcher/reporter/expression.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{


constexpr SNAP_CATCH2_NAMESPACE::reporter::operator_t const g_all_operators[] =
{
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_NULL,
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_NAMED,
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_LIST,
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY,
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_IDENTITY,
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_NEGATE,
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_ADD,
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_SUBTRACT,
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_MULTIPLY,
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_DIVIDE,
    SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_MODULO,
};


} // no name namespace



CATCH_TEST_CASE("reporter_expression", "[expression][reporter]")
{
    CATCH_START_SECTION("reporter_expression: set/get operator")
    {
        for(auto const op : g_all_operators)
        {
            SNAP_CATCH2_NAMESPACE::reporter::expression e;
            CATCH_REQUIRE(e.get_operator() == SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_NULL);
            e.set_operator(op);
            CATCH_REQUIRE(e.get_operator() == op);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_expression: ADD of two integers")
    {
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t r(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        CATCH_REQUIRE(e->get_expression_size() == 0);
        CATCH_REQUIRE(l->get_expression_size() == 0);
        CATCH_REQUIRE(r->get_expression_size() == 0);
        e->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_ADD);
        SNAP_CATCH2_NAMESPACE::reporter::token t;
        t.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_INTEGER);
        t.set_integer(55);
        l->set_token(t);
        t.set_integer(105); // expressions make a copy of the token so we can reuse it
        r->set_token(t);
        e->add_expression(l);
        e->add_expression(r);    // here 'e' represents "55 + 105"

        CATCH_REQUIRE(e->get_expression_size() == 2ULL);
        CATCH_REQUIRE(e->get_expression(0) == l);
        CATCH_REQUIRE(e->get_expression(1) == r);
        CATCH_REQUIRE(e->get_expression(0)->get_token().get_integer() == 55);
        CATCH_REQUIRE(e->get_expression(1)->get_token().get_integer() == 105);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_expression_error", "[expression][reporter][error]")
{
    CATCH_START_SECTION("reporter_expression_error: get expression out of bounds")
    {
        SNAP_CATCH2_NAMESPACE::reporter::expression e;
        CATCH_REQUIRE_THROWS_MATCHES(
                  e.get_expression(0)
                , std::overflow_error
                , Catch::Matchers::ExceptionMessage(
                          "index too large (0) to get sub-expression (max: 0)."));
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
