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
#pragma GCC diagnostic ignored "-Wfloat-equal"

// self
//
#include    "catch_main.h"


// reporter
//
#include    <eventdispatcher/reporter/token.h>



namespace
{


constexpr SNAP_CATCH2_NAMESPACE::reporter::token_t const g_all_tokens[] =
{
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_INTEGER,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_TIMESPEC,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_VARIABLE,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_OPEN_PARENTHESIS,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_CLOSE_PARENTHESIS,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_OPEN_CURLY_BRACE,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_CLOSE_CURLY_BRACE,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_COMMA,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_COLON,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EQUAL,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_PLUS,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_MINUS,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_MULTIPLY,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DIVIDE,
    SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_MODULO,
};


} // no name namespace



CATCH_TEST_CASE("reporter_token", "[token][reporter]")
{
    CATCH_START_SECTION("set/get token")
    {
        for(auto const tok : g_all_tokens)
        {
            SNAP_CATCH2_NAMESPACE::reporter::token t;
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
            t.set_token(tok);
            CATCH_REQUIRE(t.get_token() == tok);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("set/get line")
    {
        SNAP_CATCH2_NAMESPACE::reporter::token t;
        CATCH_REQUIRE(t.get_line() == 0);
        std::uint32_t line(rand());
        while(line == 0)
        {
            line = rand();
        }
        t.set_line(line);
        CATCH_REQUIRE(t.get_line() == line);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("set/get column")
    {
        SNAP_CATCH2_NAMESPACE::reporter::token t;
        CATCH_REQUIRE(t.get_column() == 0);
        std::uint32_t column(rand());
        while(column == 0)
        {
            column = rand();
        }
        t.set_column(column);
        CATCH_REQUIRE(t.get_column() == column);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("set/get integer")
    {
        for(int count(0); count < 100; ++count)
        {
            SNAP_CATCH2_NAMESPACE::reporter::token t;
            CATCH_REQUIRE(t.get_integer() == 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
            __int128 value(0);
            SNAP_CATCH2_NAMESPACE::random(value);
            t.set_integer(value);
            CATCH_REQUIRE(t.get_integer() == value);
#pragma GCC diagnostic pop
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("set/get floating point")
    {
        for(int count(0); count < 100; ++count)
        {
            SNAP_CATCH2_NAMESPACE::reporter::token t;
            CATCH_REQUIRE(t.get_floating_point() == 0.0);
            std::int64_t n(0);
            SNAP_CATCH2_NAMESPACE::random(n);
            n >>= 9;
            std::int64_t d(0);
            SNAP_CATCH2_NAMESPACE::random(d);
            d >>= 9;
            double const nominator(n);
            double denominator(d);
            if(denominator == 0.0)
            {
                denominator = 1.0;
            }
            double const value(nominator / denominator);
            t.set_floating_point(value);
            CATCH_REQUIRE(t.get_floating_point() == value);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("set/get string")
    {
        for(int count(0); count < 100; ++count)
        {
            SNAP_CATCH2_NAMESPACE::reporter::token t;
            CATCH_REQUIRE(t.get_string() == "");
            std::string const str(SNAP_CATCH2_NAMESPACE::random_string(1, 25));
            t.set_string(str);
            CATCH_REQUIRE(t.get_string() == str);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_token_error", "[token][reporter][error]")
{
    CATCH_START_SECTION("set token twice")
    {
        for(auto const tok : g_all_tokens)
        {
            SNAP_CATCH2_NAMESPACE::reporter::token t;
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
            t.set_token(tok);
            CATCH_REQUIRE(t.get_token() == tok);

            if(tok == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF
            || tok == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR)
            {
                // allowed since it's still EOF
                //
                t.set_token(tok);
                CATCH_REQUIRE(t.get_token() == tok);
            }
            else
            {
                // not allowed
                //
                CATCH_REQUIRE_THROWS_MATCHES(
                          t.set_token(tok)
                        , std::logic_error
                        , Catch::Matchers::ExceptionMessage(
                                  "trying to modify token type to something other than an error."));
            }

            // switching to an error is always allowed
            //
            t.set_token(SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("set line twice")
    {
        SNAP_CATCH2_NAMESPACE::reporter::token t;
        t.set_line(1);
        CATCH_REQUIRE(t.get_line() == 1);
        CATCH_REQUIRE_THROWS_MATCHES(
              t.set_line(2)
            , std::logic_error
            , Catch::Matchers::ExceptionMessage(
                      "trying to modify line number, not allowed anymore."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("set column twice")
    {
        SNAP_CATCH2_NAMESPACE::reporter::token t;
        t.set_column(1);
        CATCH_REQUIRE(t.get_column() == 1);
        CATCH_REQUIRE_THROWS_MATCHES(
              t.set_column(2)
            , std::logic_error
            , Catch::Matchers::ExceptionMessage(
                      "trying to modify column number, not allowed anymore."));
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
