// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// self
//
#include    "catch_main.h"


// eventdispatcher
//
#include    <eventdispatcher/dispatcher.h>
#include    <eventdispatcher/dispatcher_match.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{


void callback(ed::message & msg)
{
    snapdev::NOT_USED(msg);
}


} // no name namespace



CATCH_TEST_CASE("dispatcher_setup_error", "[dispatcher][error][setup]")
{
    CATCH_START_SECTION("create a dispatcher with callback set to nullptr")
    {
        // the callback is required
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              ::ed::define_match(
                  ::ed::Expression("REGISTER"),
                  ::ed::Callback(nullptr),
                  ::ed::MatchFunc(&ed::one_to_one_match)
              )
            , ed::parameter_error
            , Catch::Matchers::ExceptionMessage("parameter_error: a callback function is required in dispatcher_match, it cannot be set to nullptr."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("create a dispatcher with missing expression when using the one_to_one_match() function")
    {
        // the callback is required
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              ::ed::define_match(
                  ::ed::Callback(&::callback),
                  ::ed::MatchFunc(&ed::one_to_one_match)
              )
            , ed::parameter_error
            , Catch::Matchers::ExceptionMessage("parameter_error: an expression is required for the one_to_one_match()."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("create a dispatcher with missing expression when using the one_to_one_match() function")
    {
        // the priority must be between min & max
        //
        for(int count(0); count < 100; ++count)
        {
            ed::dispatcher_match::priority_t priority(0);
            for(;;)
            {
                priority = rand();
                if(priority > ed::dispatcher_match::DISPATCHER_MATCH_MAX_PRIORITY)
                {
                    break;
                }
            }
            CATCH_REQUIRE_THROWS_MATCHES(
                  ::ed::define_match(
                      ::ed::Expression("REGISTER"),
                      ::ed::Callback(&::callback),
                      ::ed::MatchFunc(&ed::one_to_one_match),
                      ::ed::Priority(priority)
                  )
                , ed::parameter_error
                , Catch::Matchers::ExceptionMessage("parameter_error: priority too large for dispatcher match."));
        }
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
