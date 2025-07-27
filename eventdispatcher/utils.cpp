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

/** \file
 * \brief Various helper functions.
 *
 * These functions are useful for our event dispatcher environment.
 */


// self
//
#include    "eventdispatcher/utils.h"

#include    "eventdispatcher/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// C++
//
#include    <cstring>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Get the current date.
 *
 * This function retrieves the current date and time with a precision
 * of microseconds.
 *
 * \todo
 * This is also defined in snap_child::get_current_date() so we should
 * unify that in some way...
 *
 * \return The time in microseconds.
 */
std::int64_t get_current_date()
{
    timeval tv;
    if(gettimeofday(&tv, nullptr) != 0)
    {
        // LCOV_EXCL_START
        int const err(errno);
        SNAP_LOG_FATAL
            << "gettimeofday() failed with errno: "
            << err
            << " ("
            << strerror(err)
            << ")"
            << SNAP_LOG_SEND;
        throw runtime_error("gettimeofday() failed");
        // LCOV_EXCL_STOP
    }

    return static_cast<int64_t>(tv.tv_sec) * static_cast<int64_t>(1'000'000)
         + static_cast<int64_t>(tv.tv_usec);
}



/** \brief Get the current date.
 *
 * This function retrieves the current date and time with a precision
 * of nanoseconds.
 *
 * \return The time in nanoseconds.
 */
std::int64_t get_current_date_ns()
{
    timespec ts;
    if(clock_gettime(CLOCK_REALTIME_COARSE, &ts) != 0)
    {
        // LCOV_EXCL_START
        int const err(errno);
        SNAP_LOG_FATAL
            << "clock_gettime() failed with errno: "
            << err
            << " ("
            << strerror(err)
            << ")"
            << SNAP_LOG_SEND;
        throw runtime_error("clock_gettime() failed");
        // LCOV_EXCL_STOP
    }

    return static_cast<int64_t>(ts.tv_sec) * static_cast<int64_t>(1'000'000'000)
         + static_cast<int64_t>(ts.tv_nsec);
}




} // namespace ed
// vim: ts=4 sw=4 et
