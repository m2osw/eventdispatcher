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
#include    <eventdispatcher/reporter/executor.h>

#include    <eventdispatcher/reporter/parser.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{



// `call()` -- DONE
// `exit()` -- PARTIAL
// `goto()` -- DONE
// `if()` -- DONE
// `label()` -- DONE
// `return()` -- DONE
// `run()` -- DONE
// `sleep()` -- DONE

constexpr char const * const g_program_sleep_func =
    "call(label: func_sleep_1s)\n"
    "exit()\n"
    "label(name: func_sleep_1s)\n"
    "sleep(seconds: 2.5)\n"
    "return()\n"
;



} // no name namespace



CATCH_TEST_CASE("reporter_executor", "[executor][reporter]")
{
    CATCH_START_SECTION("verify sleep in a function")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_sleep_func", g_program_sleep_func));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 5);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        snapdev::timespec_ex const start(snapdev::now());
        e->run();
        snapdev::timespec_ex end(snapdev::now());
        end -= start;
        CATCH_REQUIRE(end.tv_sec >= 2); // we slept for 2.5 seconds, so we expect at least start + 2 seconds
    }
    CATCH_END_SECTION()
}


//CATCH_TEST_CASE("reporter_executor_error", "[executor][reporter][error]")
//{
//    CATCH_START_SECTION("statement without instruction")
//    {
//    }
//    CATCH_END_SECTION()
//}


// vim: ts=4 sw=4 et
