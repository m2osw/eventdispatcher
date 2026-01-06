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

/** \file
 * \brief A permanent connection uses a timer when attempting to reconnect.
 *
 * This class defines an object that handles the timer delays for permanent
 * connections.
 */


// C++
//
#include    <cstdint>
#include    <string>
#include    <vector>


namespace ed
{



constexpr std::int64_t const   DEFAULT_PAUSE_BEFORE_RECONNECTING = 60LL * 1'000'000LL;  // 1 minute
constexpr char const * const   DEFAULT_PAUSE_BEFORE_RECONNECTING_STRING = "60";         // 1 minute in seconds as a string (could have multiple entries comma separated)


class pause_durations
{
public:
                                pause_durations(std::int64_t value);
                                pause_durations(std::string const & value);

    double                      initial_timer_value() const;
    double                      get_next_delay();
    void                        restart();

private:
    typedef std::vector<double> pause_t;

    void                        parse_pause_list(std::string const & pause);

    pause_t                     f_pause = pause_t();
    std::uint8_t                f_pause_pos = 1;
};



} // namespace ed
// vim: ts=4 sw=4 et
