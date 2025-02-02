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
#include    <eventdispatcher/reporter/lexer.h>


// eventdispatcher
//
#include    <eventdispatcher/exception.h>


// libexcept
//
#include    <libexcept/exception.h>


// snapdev
//
#include    <snapdev/int128_literal.h>
#include    <snapdev/ostream_int128.h>


// C++
//
#include    <cmath>


// last include
//
#include    <snapdev/poison.h>



// make literals work
//
using namespace snapdev::literals;

namespace
{



char g_white_spaces[] = {
    U'\r',
    U'\n',
    U' ',
    U'\t',
    U'\f',
};


char g_simple_tokens[] = {
    U'(',
    U')',
    U'{',
    U'}',
    U',',
    U':',
    //U'=',
    U'+',
    U'-',
    U'*',
    U'%',
};


std::string white_spaces(bool force = false, bool newlines = true)
{
    std::string result;
    int count(rand() % 30);
    if(!force)
    {
        count -= 20;
    }
    else
    {
        ++count;
    }
    if(count > 0)
    {
        for(int i(0); i < count; ++i)
        {
            if(newlines)
            {
                int const space(rand() % sizeof(g_white_spaces));
                result += g_white_spaces[space];
            }
            else
            {
                int const space(rand() % (sizeof(g_white_spaces) - 2) + 2);
                result += g_white_spaces[space];
            }
        }
    }
    return result;
}



} // no name namespace



CATCH_TEST_CASE("reporter_lexer", "[lexer][reporter]")
{
    CATCH_START_SECTION("reporter_lexer: empty input")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("empty.rprtr", "");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);

        CATCH_REQUIRE(l.get_filename() == "empty.rprtr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: white spaces only input")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("white-spaces-only.rprtr", white_spaces(true));

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: simple tokens")
    {
        for(auto const c : g_simple_tokens)
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer l("simple-token.rprtr", white_spaces() + c + white_spaces());

            SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
            CATCH_REQUIRE(t.get_token() == static_cast<SNAP_CATCH2_NAMESPACE::reporter::token_t>(c));
            t = l.next_token();
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: divide token")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("divide.rprtr",
                  white_spaces()
                + "35.3"
                + white_spaces()
                + "/"
                + white_spaces()
                + "17.2"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 35.3);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DIVIDE);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 17.2);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: simple comment")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("float-and-comment.rprtr",
                  white_spaces()
                + "45.7"
                + white_spaces()
                + "//"
                + white_spaces(false, false)        // avoid newlines in those white spaces
                + "17.2"
                + white_spaces(false, false));

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 45.7);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: divide and comments token")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("divide-and-comments.rprtr",
                  white_spaces()
                + "65.31 // this is a float\n"
                + white_spaces()
                + "/ // we want to divide it\r\n"
                + white_spaces()
                + "71.2 // by another float\n"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 65.31);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DIVIDE);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 71.2);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: hexadecimal tokens")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("divide-and-comments.rprtr",
                  "0x4511231232abcdef\n"
                + white_spaces()
                + "0XFFFabc // we want to divide it\r\n"
                + white_spaces()
                + "0x04d4b1a2 // by another float\n");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_INTEGER);
        CATCH_REQUIRE(t.get_integer() == 0x4511231232abcdef);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_INTEGER);
        CATCH_REQUIRE(t.get_integer() == 0xFFFABC);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_INTEGER);
        CATCH_REQUIRE(t.get_integer() == 0x04d4b1a2);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: NaN token")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("divide-and-comments.rprtr", "NaN\n");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(std::isnan(t.get_floating_point()));
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: compare and comments token")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("divide-and-comments.rprtr",
                  white_spaces()
                + "65.31 // this is a float\n"
                + white_spaces()
                + "<=> // we want to compare it\r\n"
                + white_spaces()
                + "-71.2 // by another float\n"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 65.31);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_COMPARE);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_MINUS);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 71.2);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: variable tokens")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("variables.rprtr",
                  white_spaces()
                + "$var // simple name\n"
                + white_spaces()
                + "$_Var123 // different characters\n"
                + white_spaces()
                + "${Quoted_Variable_3} // inside { and }\n"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_VARIABLE);
        CATCH_REQUIRE(t.get_string() == "var");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_VARIABLE);
        CATCH_REQUIRE(t.get_string() == "_Var123");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_VARIABLE);
        CATCH_REQUIRE(t.get_string() == "Quoted_Variable_3");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: date tokens")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("dates.rprtr",
                  white_spaces()
                + "@1710686374.536271827 // %s.%N timespec\n"
                + white_spaces()
                + "@\"03/17/2024 14:35:22\" // double quote %D %T\n"
                + white_spaces()
                + "@'05/29/2023 07:41:23' // single quote %D %T\n"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_TIMESPEC);
        CATCH_REQUIRE(t.get_integer() == 0x65F700A6000000001FF6DBD3_int128);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_TIMESPEC);
//std::cerr << "2024 time: " << std::hex << t.get_integer() << "\n";
        CATCH_REQUIRE(t.get_integer() == 0x65F6FFAA0000000000000000_int128);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_TIMESPEC);
//std::cerr << "2023 time: " << std::hex << t.get_integer() << "\n";
        CATCH_REQUIRE(t.get_integer() == 0x647457230000000000000000_int128);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: IP tokens")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("ips.rprtr",
                  white_spaces()
                + "<128.71.3.227> // IPv4 (no port means port 0)\n"
                + white_spaces()
                + "<127.0.4.127:8080> // IPv4 with a port\n"
                + white_spaces()
                + "<200.6.7.98:443> // another IPv4 with a port\n"
                + white_spaces()
                + "<*:53> // localhost IPv4/6 with a port, output as IPv6\n"
                + white_spaces()
                + "<[feff::9ab:32:1b6]:2424> // IPv6\n"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS);
        CATCH_REQUIRE(t.get_string() == "128.71.3.227:0");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS);
        CATCH_REQUIRE(t.get_string() == "127.0.4.127:8080");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS);
        CATCH_REQUIRE(t.get_string() == "200.6.7.98:443");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS);
        CATCH_REQUIRE(t.get_string() == "[::1]:53");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ADDRESS);
        CATCH_REQUIRE(t.get_string() == "[feff::9ab:32:1b6]:2424");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: double string tokens")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("strings.rprtr",
                  white_spaces()
                + "\"\" // empty string\n"
                + white_spaces()
                + "\"simple\"\n"
                + white_spaces()
                + "\"newline \\n\"\n"
                + white_spaces()
                + "\"carriage return \\r\"\n"
                + white_spaces()
                + "\"both \\r\\n\"\n"
                + white_spaces()
                + "\"backspace \\b\"\n"
                + white_spaces()
                + "\"bell \\a\"\n"
                + white_spaces()
                + "\"formfeed \\f\"\n"
                + white_spaces()
                + "\"tab \\t\"\n"
                + white_spaces()
                + "\"vertical tab \\v\"\n"
                + white_spaces()
                + "\"others \\\\ \\\" \\' \\`\"\n"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "simple");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "newline \n");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "carriage return \r");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "both \r\n");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "backspace \b");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "bell \a");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "formfeed \f");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "tab \t");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "vertical tab \v");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "others \\ \" ' `");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: currently unsupported backslash tokens")
    {
        // at this point the following are not implemented
        //
        char unimplemented[] = {
            'x', 'u', 'U',
            '0', '1', '2', '3', '4',
            '5', '6', '7', '8', '9',
        };
        for(auto const c : unimplemented)
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer l("backslashes.rprtr", std::string("test: \"\\") + c + "5\"");
            SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
            CATCH_REQUIRE(t.get_string() == "test");
            t = l.next_token();
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_COLON);
            CATCH_REQUIRE_THROWS_MATCHES(
                  l.next_token()
                , libexcept::fixme
                , Catch::Matchers::ExceptionMessage("fixme: sorry, the \\... with a number to define a character are not yet supported."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: single string tokens")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("strings.rprtr",
                  white_spaces()
                + "'' // empty string\n"
                + white_spaces()
                + "'simple'\n"
                + white_spaces()
                + "'newline \\n'\n"
                + white_spaces()
                + "'carriage return \\r'\n"
                + white_spaces()
                + "'both \\r\\n'\n"
                + white_spaces()
                + "'backspace \\b'\n"
                + white_spaces()
                + "'bell \\a'\n"
                + white_spaces()
                + "'formfeed \\f'\n"
                + white_spaces()
                + "'tab \\t'\n"
                + white_spaces()
                + "'vertical tab \\v'\n"
                + white_spaces()
                + "'others \\\\ \\\" \\' \\`'\n"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "simple");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "newline \n");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "carriage return \r");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "both \r\n");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "backspace \b");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "bell \a");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "formfeed \f");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "tab \t");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "vertical tab \v");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_SINGLE_STRING);
        CATCH_REQUIRE(t.get_string() == "others \\ \" ' `");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: integer tokens")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("integers.rprtr",
                  white_spaces()
                + "0\n"
                + white_spaces()
                + "1001\n"
                + white_spaces()
                + "-34\n"
                + white_spaces()
                + "+99\n"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_INTEGER);
        CATCH_REQUIRE(t.get_integer() == 0);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_INTEGER);
        CATCH_REQUIRE(t.get_integer() == 1001);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_MINUS);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_INTEGER);
        CATCH_REQUIRE(t.get_integer() == 34);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_PLUS);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_INTEGER);
        CATCH_REQUIRE(t.get_integer() == 99);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: floating point tokens")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("floating-points.rprtr",
                  white_spaces()
                + "3.\n"
                + white_spaces()
                + ".7\n"
                + white_spaces()
                + "10.01\n"
                + white_spaces()
                + "-34e-34\n"
                + white_spaces()
                + "+99e+3\n"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 3.0);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 0.7);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 10.01);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_MINUS);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 34e-34);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_PLUS);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_FLOATING_POINT);
        CATCH_REQUIRE(t.get_floating_point() == 99e+3);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer: identifier tokens")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("identifiers.rprtr",
                  white_spaces()
                + "simple\n"
                + white_spaces()
                + "TEST\n"
                + white_spaces()
                + "_underscore\n"
                + white_spaces()
                + "Number123\n"
                + white_spaces()
                + "Inside_Underscore\n"
                + white_spaces()
                + "End_\n"
                + white_spaces());

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "simple");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "TEST");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "_underscore");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "Number123");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "Inside_Underscore");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "End_");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_lexer_file", "[lexer][reporter][file]")
{
    CATCH_START_SECTION("reporter_lexer_file: file does not exist")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(SNAP_CATCH2_NAMESPACE::reporter::create_lexer("unknown.rprtr"));
        CATCH_REQUIRE(l == nullptr);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_file: exact filename")
    {
        std::string const source_dir(SNAP_CATCH2_NAMESPACE::g_source_dir());
        std::string const filename(source_dir + "/tests/rprtr/test_load_with_create_lexer.rprtr"); // include the extension
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(SNAP_CATCH2_NAMESPACE::reporter::create_lexer(filename));
        CATCH_REQUIRE(l != nullptr);

        // print(message: "it worked.")
        SNAP_CATCH2_NAMESPACE::reporter::token t(l->next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "print");
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_OPEN_PARENTHESIS);
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "message");
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_COLON);
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "it worked.");
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_CLOSE_PARENTHESIS);
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_file: filename without extension")
    {
        std::string const source_dir(SNAP_CATCH2_NAMESPACE::g_source_dir());
        std::string const filename(source_dir + "/tests/rprtr/test_load_with_create_lexer"); // exclude the extension
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(SNAP_CATCH2_NAMESPACE::reporter::create_lexer(filename));
        CATCH_REQUIRE(l != nullptr);

        // print(message: "it worked.")
        SNAP_CATCH2_NAMESPACE::reporter::token t(l->next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "print");
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_OPEN_PARENTHESIS);
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "message");
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_COLON);
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_DOUBLE_STRING);
        CATCH_REQUIRE(t.get_string() == "it worked.");
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_CLOSE_PARENTHESIS);
        t = l->next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_lexer_error", "[lexer][reporter][error]")
{
    CATCH_START_SECTION("reporter_lexer_error: unterminated string")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("unterminated-string.rprtr", "\"unterminated");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: multi-line string")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("multi-line-string.rprtr", "\"multi\nline\"");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "line");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: unterminated string in backslash case")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("unterminated-backslash.rprtr",
            "\"string with \\");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: empty unquoted variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("empty-variable.rprtr", "empty $ variable name");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "empty");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "variable");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "name");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: empty quoted variable")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("empty-quoted-variable.rprtr", "empty ${} quoted variable name");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "empty");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "quoted");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "variable");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "name");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: invalid quoted variable name")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("invalid-variable-name.rprtr", "${bad name}");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
        CATCH_REQUIRE(t.get_string() == "name");
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_CLOSE_CURLY_BRACE);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: empty date (double quote)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("unterminated-date.rprtr", "@\"\"");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: empty date (single quote)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("unterminated-date.rprtr", "@''");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: unterminated date")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("unterminated-date.rprtr", "@\"unterminated");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: unterminated IP")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("unterminated-ip.rprtr", "<128.71.3.227");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: bad IP (bad name)")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("unterminated-ip.rprtr", "<some bad IP address>");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: empty IP")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("empty-ip.rprtr", "<>");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: IP range is not available")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("ip-range.rprtr", "<10.0.1.0-10.0.1.255>");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: no from IP")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("no-from-ip.rprtr", "<-10.0.1.255>");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: bad integer")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("bad-integers.rprtr",
            "10000000000000000000\n"
            "1-1\n"
            "1+1\n");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        for(int i(0); i < 2; ++i)
        {
            t = l.next_token();
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        }
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: bad floating points")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("bad-floating-points.rprtr",
            "3.3e+\n"
            "3.3e++5\n"
            "3.3ee+5\n"
            "3.3EE+5\n"
            "3.3EE++5\n"
            "3e++5\n"
            "3ee+5\n"
            "3EE+5\n"
            "3EE++5\n"
            "3.3e-\n"
            "3.3e--5\n"
            "3.3ee-5\n"
            "3.3EE-5\n"
            "3.3EE--5\n"
            "3e--5\n"
            "3ee-5\n"
            "3EE-5\n"
            "3EE--5\n"
            "3..3e-3\n"
            "3.3.e-5\n"
            "3.3e.+6\n"
            "3.3e-.5\n"
            "3.3e9.\n");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        for(int i(0); i < 22; ++i)
        {
            t = l.next_token();
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        }
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: variable name cannot start with digit")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("unexpected-digit.rprtr",
            "$5var\n"
            "${0digits_allowed}\n");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        for(int i(0); i < 1; ++i)
        {
            t = l.next_token();
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        }
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: unexpected character")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer l("unexpected-character.rprtr",
            "\\\n"
            "#\n");

        SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        for(int i(0); i < 1; ++i)
        {
            t = l.next_token();
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_ERROR);
        }
        t = l.next_token();
        CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_EOF);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: unsupported backslash tokens")
    {
        // there are likely never going to ever be supported
        //
        char unimplemented[] = {
            'q', 'z',
        };
        for(auto const c : unimplemented)
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer l("backslashes.rprtr", std::string("test: \"\\") + c + "5\"");
            SNAP_CATCH2_NAMESPACE::reporter::token t(l.next_token());
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_IDENTIFIER);
            CATCH_REQUIRE(t.get_string() == "test");
            t = l.next_token();
            CATCH_REQUIRE(t.get_token() == SNAP_CATCH2_NAMESPACE::reporter::token_t::TOKEN_COLON);
            CATCH_REQUIRE_THROWS_MATCHES(
                  l.next_token()
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage(std::string("event_dispatcher_exception: invalid escape character '") + c + "'."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("reporter_lexer_error: invalid hexadecimal number")
    {
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer l("backslashes.rprtr", "0x");
            CATCH_REQUIRE_THROWS_MATCHES(
                  l.next_token()
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: invalid hexadecimal number, at least one digits was expected."));
        }
        {
            SNAP_CATCH2_NAMESPACE::reporter::lexer l("backslashes.rprtr", "0X");
            CATCH_REQUIRE_THROWS_MATCHES(
                  l.next_token()
                , ed::runtime_error
                , Catch::Matchers::ExceptionMessage("event_dispatcher_exception: invalid hexadecimal number, at least one digits was expected."));
        }
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
