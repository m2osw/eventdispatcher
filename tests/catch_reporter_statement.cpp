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

// this diagnostic has to be turned off "globally" so the catch2 does not
// generate the warning on the floating point == operator
//
#pragma GCC diagnostic ignored "-Wfloat-equal"

// self
//
#include    "catch_main.h"


// reporter
//
#include    <eventdispatcher/reporter/statement.h>

#include    <eventdispatcher/reporter/instruction_factory.h>


// eventdispatcher
//
#include    <eventdispatcher/exception.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{




} // no name namespace



CATCH_TEST_CASE("reporter_statement", "[statement][reporter]")
{
    CATCH_START_SECTION("reporter_statement: verify basic program")
    {
        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t if_inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("if"));
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(if_inst));
        CATCH_REQUIRE(s->get_instruction() == if_inst);

        s->set_filename("this-filename.rprtr");
        CATCH_REQUIRE(s->get_filename() == "this-filename.rprtr");

        s->set_line(1041);
        CATCH_REQUIRE(s->get_line() == 1041);

        CATCH_REQUIRE(s->get_location() == "this-filename.rprtr:1041: ");

        // create an expression with 3.1 + 2.9
        //
        SNAP_CATCH2_NAMESPACE::reporter::token t1;
        t1.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        t1.set_string("unordered_label");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t c1(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        c1->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        c1->set_token(t1);
        s->add_parameter("unordered", c1);

        SNAP_CATCH2_NAMESPACE::reporter::token t2;
        t2.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        t2.set_string("less_label");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t c2(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        c2->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        c2->set_token(t2);
        s->add_parameter("less", c2);

        SNAP_CATCH2_NAMESPACE::reporter::token t3;
        t3.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        t3.set_string("equal_label");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t c3(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        c3->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        c3->set_token(t3);
        s->add_parameter("equal", c3);

        SNAP_CATCH2_NAMESPACE::reporter::token t4;
        t4.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        t4.set_string("greater_label");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t c4(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        c4->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        c4->set_token(t4);
        s->add_parameter("greater", c4);

        CATCH_REQUIRE(s->get_parameter("unordered") == c1);
        CATCH_REQUIRE(s->get_parameter("less") == c2);
        CATCH_REQUIRE(s->get_parameter("equal") == c3);
        CATCH_REQUIRE(s->get_parameter("greater") == c4);
        CATCH_REQUIRE(s->get_parameter("not-added") == SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t());
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_statement_error", "[statement][reporter][error]")
{
    CATCH_START_SECTION("reporter_statement_error: statement without instruction")
    {
        CATCH_REQUIRE_THROWS_MATCHES(
              SNAP_CATCH2_NAMESPACE::reporter::statement(nullptr)
            , ed::implementation_error
            , Catch::Matchers::ExceptionMessage(
                      "implementation_error: an instruction must always be attached to a statement."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_statement_error: parameter already defined")
    {
        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t print_inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("print"));
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(print_inst));
        CATCH_REQUIRE(s->get_instruction() == print_inst);

        SNAP_CATCH2_NAMESPACE::reporter::token t1;
        t1.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        t1.set_string("this is our message");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t c1(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        c1->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        c1->set_token(t1);
        s->add_parameter("message", c1);

        CATCH_REQUIRE_THROWS_MATCHES(
              s->add_parameter("message", c1)
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: parameter \"message\" defined more than once."));

        s->verify_parameters();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_statement_error: unknown parameter")
    {
        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t print_inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("print"));
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(print_inst));
        CATCH_REQUIRE(s->get_instruction() == print_inst);

        SNAP_CATCH2_NAMESPACE::reporter::token t1;
        t1.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        t1.set_string("this could be anything really");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t c1(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        c1->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        c1->set_token(t1);

        CATCH_REQUIRE_THROWS_MATCHES(
              s->add_parameter("unknown", c1)
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: parameter \"unknown\" not accepted by \"print\"."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_statement_error: missing parameter")
    {
        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t print_inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("print"));
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(print_inst));
        CATCH_REQUIRE(s->get_instruction() == print_inst);

        //SNAP_CATCH2_NAMESPACE::reporter::token t1;
        //t1.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        //t1.set_string("this could be anything really");
        //SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t c1(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        //c1->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        //c1->set_token(t1);

        CATCH_REQUIRE_THROWS_MATCHES(
              s->verify_parameters()
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: parameter \"message\" is required by \"print\"."));
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
