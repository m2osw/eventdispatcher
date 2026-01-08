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
 * \brief Implementation of the duration manager.
 *
 * The pause_durations class is used to define a set of delays to wait for
 * a permanent connection to reconnect. The idea is to have a series of
 * increasing values.
 */


// self
//
#include    "eventdispatcher/pause_durations.h"

//#include    "eventdispatcher/communicator.h"
#include    "eventdispatcher/exception.h"
//#include    "eventdispatcher/tcp_server_client_message_connection.h"
//#include    "eventdispatcher/thread_done_signal.h"


// snaplogger
//
//#include    <snaplogger/message.h>


// snapdev
//
//#include    <snapdev/not_used.h>


// advgetopt
//
#include    <advgetopt/utils.h>
#include    <advgetopt/validator_duration.h>


// cppthread
//
//#include    <cppthread/exception.h>
//#include    <cppthread/guard.h>
//#include    <cppthread/runner.h>
//#include    <cppthread/thread.h>


// C++
//
#include    <cmath>


// C
//
//#include    <sys/socket.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



pause_durations::pause_durations(std::int64_t value)
{
    f_pause.push_back(static_cast<double>(value) / 1'000'000.0);
    restart();
}


pause_durations::pause_durations(std::string const & value)
{
    parse_pause_list(value);
    restart();
}


void pause_durations::parse_pause_list(std::string const & pause)
{
    advgetopt::string_list_t durations;
    advgetopt::split_string(pause, durations, {{","}});
    for(auto const & d : durations)
    {
        if(f_pause.size() >= 254)
        {
            throw invalid_parameter("too many pause durations, limit is 255.");
        }

        double seconds(0);
        if(!advgetopt::validator_duration::convert_string(
                  d
                , advgetopt::validator_duration::VALIDATOR_DURATION_DEFAULT_FLAGS
                , seconds))
        {
            throw invalid_parameter(
                  "pause duration \""
                + d
                + "\" is not valid.");
        }
        f_pause.push_back(seconds);
    }

    // get at least one entry to make it simpler to handle
    //
    if(f_pause.empty())
    {
        f_pause.push_back(static_cast<double>(DEFAULT_PAUSE_BEFORE_RECONNECTING) / 1'000'000.0);
    }
}


double pause_durations::initial_timer_value() const
{
    return f_pause[0] < 0.0 ? -f_pause[0] : 0.0;
}


double pause_durations::get_next_delay()
{
    if(f_pause_pos < f_pause.size())
    {
        double const delay(fabs(f_pause[f_pause_pos]));
        ++f_pause_pos;
        return delay;
    }

    return -1.0;
}


void pause_durations::restart()
{
    if(f_pause.size() == 1
    || f_pause[0] > 0.0)
    {
        f_pause_pos = 0;
    }
    else
    {
        f_pause_pos = 1;
    }
}



} // namespace ed
// vim: ts=4 sw=4 et
