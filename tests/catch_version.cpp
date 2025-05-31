// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// self
//
#include    "catch_main.h"


// eventdispatcher
//
#include    <eventdispatcher/version.h>


// cppprocess
//
#include    <cppprocess/version.h>


// last include
//
#include    <snapdev/poison.h>




CATCH_TEST_CASE("Version", "[version]")
{
    CATCH_START_SECTION("verify runtime vs compile time eventdispatcher version numbers")
    {
        CATCH_REQUIRE(ed::get_major_version()   == EVENTDISPATCHER_VERSION_MAJOR);
        CATCH_REQUIRE(ed::get_release_version() == EVENTDISPATCHER_VERSION_MINOR);
        CATCH_REQUIRE(ed::get_patch_version()   == EVENTDISPATCHER_VERSION_PATCH);
        CATCH_REQUIRE(strcmp(ed::get_version_string(), EVENTDISPATCHER_VERSION_STRING) == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify runtime vs compile time cppprocess version numbers")
    {
        CATCH_REQUIRE(cppprocess::get_major_version()   == CPPPROCESS_VERSION_MAJOR);
        CATCH_REQUIRE(cppprocess::get_release_version() == CPPPROCESS_VERSION_MINOR);
        CATCH_REQUIRE(cppprocess::get_patch_version()   == CPPPROCESS_VERSION_PATCH);
        CATCH_REQUIRE(strcmp(cppprocess::get_version_string(), CPPPROCESS_VERSION_STRING) == 0);
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
