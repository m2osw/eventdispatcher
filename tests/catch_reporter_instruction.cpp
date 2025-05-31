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

/** \file
 * \brief This test manually checks a few core instructions.
 *
 * The main test to verify all the instructions is the executor one
 * (catch_reportor_executor.cpp). This test only verifies that we
 * can create a program and execute it step by step without using
 * the executor.
 *
 * The executor has many programs that are used to make sure that
 * all the instructions work as expected.
 */

// self
//
#include    "catch_main.h"


// reporter
//
#include    <eventdispatcher/reporter/instruction_factory.h>

#include    <eventdispatcher/reporter/state.h>
#include    <eventdispatcher/reporter/variable_string.h>


// eventdispatcher
//
#include    <eventdispatcher/exception.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{




} // no name namespace



CATCH_TEST_CASE("reporter_instruction", "[instruction][reporter]")
{
    CATCH_START_SECTION("reporter_instruction: check label")
    {
        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t label(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("label"));
        CATCH_REQUIRE(label != nullptr);
        CATCH_REQUIRE(label->get_name() == "label");
        SNAP_CATCH2_NAMESPACE::reporter::state s;
        //SNAP_CATCH2_NAMESPACE::reporter::state const safe_copy(s);
        label->func(s);
        //CATCH_REQUIRE(safe_copy == s); -- TODO?
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction: check goto")
    {
        SNAP_CATCH2_NAMESPACE::reporter::state s;
        CATCH_REQUIRE(s.get_statement_size() == 0);

        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("label"));
        CATCH_REQUIRE(inst != nullptr);
        CATCH_REQUIRE(inst->get_name() == "label");
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(inst));
        SNAP_CATCH2_NAMESPACE::reporter::token t;
        t.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        t.set_string("start");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        e->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        e->set_token(t);
        stmt->add_parameter("name", e);
        s.add_statement(stmt);
        CATCH_REQUIRE(s.get_statement_size() == 1);

        inst = SNAP_CATCH2_NAMESPACE::reporter::get_instruction("goto");
        CATCH_REQUIRE(inst != nullptr);
        CATCH_REQUIRE(inst->get_name() == "goto");
        stmt = std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(inst);
        e = std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>();
        e->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        e->set_token(t);
        stmt->add_parameter("label", e);
        s.add_statement(stmt);
        CATCH_REQUIRE(s.get_statement_size() == 2);

        // in this case we expect the state to include parameters (variables)
        // that were computed from the statement parameter (expressions)
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable_string::pointer_t var_name(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::variable_string>("label"));
        var_name->set_string("start");
        CATCH_REQUIRE(var_name->get_type() == "string");
        s.add_parameter(var_name);

        // execute the goto
        //
        s.set_ip(1);
        CATCH_REQUIRE(s.get_ip() == 1); // goto is at position 1
        inst->func(s);
        CATCH_REQUIRE(s.get_ip() == 0); // back to 0 after the goto

        CATCH_REQUIRE(s.get_parameter("label") != nullptr);
        s.clear_parameters();
        CATCH_REQUIRE(s.get_parameter("label") == nullptr);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction: global variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::state s;
        CATCH_REQUIRE(s.get_variable("global") == nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::variable_string>("global"));
        CATCH_REQUIRE(var != nullptr);
        //var->set_string("value");
        s.set_variable(var);
        CATCH_REQUIRE(s.get_variable("global") == var);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction: check call/return")
    {
        SNAP_CATCH2_NAMESPACE::reporter::state s;

        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t call_inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("call"));
        CATCH_REQUIRE(call_inst != nullptr);
        CATCH_REQUIRE(call_inst->get_name() == "call");
        SNAP_CATCH2_NAMESPACE::reporter::token t1;
        t1.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        t1.set_string("func_sample");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t e1(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        e1->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        e1->set_token(t1);
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t call_stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(call_inst));
        call_stmt->add_parameter("label", e1);
        s.add_statement(call_stmt);

        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t exit_inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("exit"));
        CATCH_REQUIRE(exit_inst != nullptr);
        CATCH_REQUIRE(exit_inst->get_name() == "exit");
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t exit_stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(exit_inst));
        s.add_statement(exit_stmt);

        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t label_inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("label"));
        CATCH_REQUIRE(label_inst != nullptr);
        CATCH_REQUIRE(label_inst->get_name() == "label");
        SNAP_CATCH2_NAMESPACE::reporter::token t2;
        t2.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        t2.set_string("func_sample");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t e2(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        e2->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        e2->set_token(t2);
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t label_stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(label_inst));
        label_stmt->add_parameter("name", e2);
        s.add_statement(label_stmt);

        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t return_inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("return"));
        CATCH_REQUIRE(return_inst != nullptr);
        CATCH_REQUIRE(return_inst->get_name() == "return");
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t return_stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(return_inst));
        s.add_statement(return_stmt);

        // execute CALL func_sample
        //
        CATCH_REQUIRE(s.get_ip() == 0);
        SNAP_CATCH2_NAMESPACE::reporter::variable_string::pointer_t var_name(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::variable_string>("label"));
        var_name->set_string("func_sample");
        CATCH_REQUIRE(var_name->get_type() == "string");
        s.add_parameter(var_name);
        s.set_ip(1);        // TODO: somehow the executor will increase IP before calls to func()
        call_inst->func(s);

        // execute RETURN
        //
        s.clear_parameters();
        CATCH_REQUIRE(s.get_ip() == 2);
        return_inst->func(s);

        // execute EXIT
        //
        s.clear_parameters();
        CATCH_REQUIRE(s.get_ip() == 1);
        exit_inst->func(s);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_instruction_error", "[instruction][reporter][error]")
{
    CATCH_START_SECTION("reporter_instruction_error: get unknown instruction")
    {
        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t unknown(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("unknown_instruction"));
        CATCH_REQUIRE(unknown == nullptr);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction_error: search non-existant label")
    {
        SNAP_CATCH2_NAMESPACE::reporter::state s;
        CATCH_REQUIRE_THROWS_MATCHES(
              s.get_label_position("unknown")
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: label \"unknown\" not found."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction_error: search non-existant parameter")
    {
        SNAP_CATCH2_NAMESPACE::reporter::state s;
        CATCH_REQUIRE(s.get_parameter("unknown") == nullptr);
        CATCH_REQUIRE(s.get_parameter("unknown", false) == nullptr);
        CATCH_REQUIRE_THROWS_MATCHES(
              s.get_parameter("unknown", true)
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: parameter \"unknown\" is required."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction_error: label without a \"name\" parameter (missing)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::state s;

        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("label"));
        CATCH_REQUIRE(inst != nullptr);
        CATCH_REQUIRE(inst->get_name() == "label");
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(inst));

        CATCH_REQUIRE_THROWS_MATCHES(
              s.add_statement(stmt)
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: the \"name\" parameter of the \"label\" statement is mandatory."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction_error: label without a \"name\" parameter (misspelled)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::state s;

        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("label"));
        CATCH_REQUIRE(inst != nullptr);
        CATCH_REQUIRE(inst->get_name() == "label");
        SNAP_CATCH2_NAMESPACE::reporter::token t;
        t.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        t.set_string("start");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        e->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        e->set_token(t);
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(inst));

        CATCH_REQUIRE_THROWS_MATCHES(
              stmt->add_parameter("names", e) // misspelled ("names" instead of "name")
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: parameter \"names\" not accepted by \"label\"."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction_error: label with name parameter not of type PRIMARY")
    {
        SNAP_CATCH2_NAMESPACE::reporter::state s;

        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("label"));
        CATCH_REQUIRE(inst != nullptr);
        CATCH_REQUIRE(inst->get_name() == "label");
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        e->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_ADD);
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(inst));
        stmt->add_parameter("name", e);

        CATCH_REQUIRE_THROWS_MATCHES(
              s.add_statement(stmt)
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: the value of the \"name\" parameter of the \"label\" statement cannot be dynamically computed."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction_error: label with a name parameter of type INTEGER")
    {
        SNAP_CATCH2_NAMESPACE::reporter::state s;

        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("label"));
        CATCH_REQUIRE(inst != nullptr);
        CATCH_REQUIRE(inst->get_name() == "label");
        SNAP_CATCH2_NAMESPACE::reporter::token t;
        t.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_INTEGER);
        t.set_integer(123);
        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        e->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        e->set_token(t);
        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(inst));
        stmt->add_parameter("name", e);

        CATCH_REQUIRE_THROWS_MATCHES(
              s.add_statement(stmt)
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: the value of the \"name\" parameter of the \"label\" statement must be an identifier."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction_error: label already defined")
    {
        SNAP_CATCH2_NAMESPACE::reporter::state s;

        {
            SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("label"));
            CATCH_REQUIRE(inst != nullptr);
            CATCH_REQUIRE(inst->get_name() == "label");
            SNAP_CATCH2_NAMESPACE::reporter::token t;
            t.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
            t.set_string("duplicate");
            SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
            e->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
            e->set_token(t);
            SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(inst));
            stmt->add_parameter("name", e);

            // second time with the same name fails
            //
            CATCH_REQUIRE_THROWS_MATCHES(
                  stmt->add_parameter("name", e)
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "event_dispatcher_exception: parameter \"name\" defined more than once."));

            s.add_statement(stmt);

            CATCH_REQUIRE(s.get_statement_size() == 1);
            CATCH_REQUIRE(s.get_statement(0) == stmt);
        }

        // trying to add another label with the same name fails
        {
            SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("label"));
            CATCH_REQUIRE(inst != nullptr);
            CATCH_REQUIRE(inst->get_name() == "label");
            SNAP_CATCH2_NAMESPACE::reporter::token t;
            t.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
            t.set_string("duplicate");
            SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
            e->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
            e->set_token(t);
            SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(inst));
            stmt->add_parameter("name", e);

            CATCH_REQUIRE_THROWS_MATCHES(
                  s.add_statement(stmt)
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage(
                          "event_dispatcher_exception: label \"duplicate\" already defined at position 0."));

            CATCH_REQUIRE(s.get_statement_size() == 1);
            CATCH_REQUIRE(s.get_statement(0) != stmt);
            CATCH_REQUIRE_THROWS_MATCHES(
                  s.get_statement(1)
                , ed::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: ip out of program not allowed."));
        }

        // make sure the second statement did not make it through
        //
        s.set_ip(0);
        s.set_ip(1);    // exit() does this!
        CATCH_REQUIRE_THROWS_MATCHES(
              s.set_ip(2)       // this is a bug
            , ed::out_of_range
            , Catch::Matchers::ExceptionMessage(
                      "out_of_range: ip out of program not allowed."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction_error: \"return()\" does not accept any parameters")
    {
        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("return"));
        CATCH_REQUIRE(inst != nullptr);
        CATCH_REQUIRE(inst->get_name() == "return");

        SNAP_CATCH2_NAMESPACE::reporter::token t;
        t.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        t.set_string("duplicate");

        SNAP_CATCH2_NAMESPACE::reporter::expression::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::expression>());
        e->set_operator(SNAP_CATCH2_NAMESPACE::reporter::operator_t::OPERATOR_PRIMARY);
        e->set_token(t);

        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t stmt(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::statement>(inst));
        CATCH_REQUIRE_THROWS_MATCHES(
              stmt->add_parameter("void", e)
            , ed::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "event_dispatcher_exception: parameter \"void\" not accepted by \"return\"."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_instruction_error: \"run()\" cannot be called")
    {
        SNAP_CATCH2_NAMESPACE::reporter::instruction::pointer_t inst(SNAP_CATCH2_NAMESPACE::reporter::get_instruction("run"));
        CATCH_REQUIRE(inst != nullptr);
        CATCH_REQUIRE(inst->get_name() == "run");

        SNAP_CATCH2_NAMESPACE::reporter::state s;
        CATCH_REQUIRE_THROWS_MATCHES(
              inst->func(s)
            , ed::implementation_error
            , Catch::Matchers::ExceptionMessage(
                      "implementation_error: run::func() was called when it should be intercepted by the executor."));
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
