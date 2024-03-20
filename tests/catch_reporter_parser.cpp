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
//#pragma GCC diagnostic ignored "-Wfloat-equal"

// self
//
#include    "catch_main.h"


// reporter
//
#include    <eventdispatcher/reporter/parser.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{


// test all internal instructions
//
constexpr char const * const g_program1 =
    "call(label: function)\n"
    "exit()\n"
    "goto(label: over_here)\n"
    "if(less: when_smaller, equal: when_equal, greater: when_larger)\n"
    "label(name: label_name)\n"
    "verify_message(sent_server: name,"
                " required_parameters: { color: orange, timeout: 1 + 57 * (3600 / 3) % 7200 - 34, length: -567, height: +33.21 },"
                " optional_parameters: {},"
                " forbidden_parameters: { hurray })\n"
;


} // no name namespace



CATCH_TEST_CASE("reporter_parser", "[parser][reporter]")
{
    CATCH_START_SECTION("parse program1")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program1", g_program1));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 6);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_parser_error", "[parser][reporter][error]")
{
    CATCH_START_SECTION("bad variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("bad_variable.rptr", "${bad_var"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "invalid token."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("identifier expected for instruction")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("not_identifier.rptr", "exit() 123()"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "expected identifier."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("unknown instruction")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("unknown_instruction.rptr", "unknown_instruction()"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "unknown instruction."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("expect '(' after instruction")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("missing_open_parenthesis_EOF.rptr", "exit"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "expected '(' parenthesis instead of EOF."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("expect '(' not another token")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("missing_open_parenthesis.rptr", "exit 123"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "expected '(' parenthesis."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("expect ')' before EOF")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("missing_close_parenthesis.rptr", "exit("));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "expected ')' parenthesis instead of EOF."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("expect ')' to end list of parameters")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("missing_close_parenthesis.rptr", "exit(error_message: \"msg\"}"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "expected ')' parenthesis to end parameter list."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parameter name not identifier")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("parameter_name_not_identifier.rptr", "exit(123: \"msg\"}"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "expected identifier to name parameter."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("colon missing after parameter name EOF")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("parameter_name_no_colon.rptr", "exit(error_message"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "expected ':' after parameter name, not EOF."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("colon missing after parameter name")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("parameter_name_no_colon.rptr", "exit(error_message \"msg\")"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "expected ':' after parameter name."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parameter expression missing")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("parameter_without_expression.rptr", "exit(error_message:"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "expected expression."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("list must end with '}', not EOF")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("list_end_with_curly_bracket.rptr", "verify_message(required_parameters: { version: 123, "));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "end of file found before end of list ('}' missing)."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("list must end with '}'")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("list_end_with_curly_bracket.rptr", "verify_message(required_parameters: { version: 123 )"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "a list of parameter values must end with '}'."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("name of list item must be an identifier")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("list_item_identifier.rptr", "verify_message(required_parameters: { 123: version )"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "a list item must be named using an identifier."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("unterminated list item (EOF early)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("list_item_identifier.rptr", "verify_message(required_parameters: { version "));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "a list must end with a '}'."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("list item expression missing (EOF early)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("list_item_identifier.rptr", "verify_message(required_parameters: { version : "));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "a list item with a colon (:) must be followed by an expression."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("expression open parenthesis and EOF")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("expression_parenthesis_eof.rptr", "exit(error_message: ("));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "an expression between parenthesis must include at least one primary expression."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("expression close parenthesis missing")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("expression_parenthesis_missing.rptr", "verify_message(required_parameters: { color: ( 234 + 770 }"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "an expression between parenthesis must include the ')' at the end."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("expression primary not found")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("expression_primary_missing.rptr", "verify_message(required_parameters: { color: ( { oops - si } ))"));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        CATCH_REQUIRE_THROWS_MATCHES(
              p->parse_program()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "expected a primary token for expression."));
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
