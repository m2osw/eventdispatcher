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
#pragma once

// catch2
//
#include    <catch2/snapcatch2.hpp>


// snapdev
//
#include    <snapdev/mkdir_p.h>


// C++
//
#include    <string>
#include    <cstring>
#include    <cstdlib>
#include    <iostream>



namespace SNAP_CATCH2_NAMESPACE
{



extern char ** g_argv;



inline char32_t rand_char(bool full_range = false)
{
    char32_t const max((full_range ? 0x0110000 : 0x0010000) - (0xE000 - 0xD800));

    char32_t wc;
    do
    {
        wc = ((rand() << 16) ^ rand()) % max;
    }
    while(wc == 0);
    if(wc >= 0xD800)
    {
        // skip the surrogates
        //
        wc += 0xE000 - 0xD800;
    }

    return wc;
}


inline std::string get_tmp_dir(std::string const & subdir)
{
    std::string const & tmp_dir(SNAP_CATCH2_NAMESPACE::g_tmp_dir() + "/" + subdir);

    CATCH_REQUIRE(snapdev::mkdir_p(tmp_dir, false, 0700) == 0);

    return tmp_dir;
}




}
// namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
