// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Implementation of the Timer class.
 *
 * This class allows you to create a "connection" which is just a timer.
 *
 * All connections have a timer feature, but at times you have to either
 * disable a connection or you already use the timer for some other
 * reasons so we offer a separate timer class for you additional needs.
 */


// self
//
#include    "eventdispatcher/timer.h"

#include    "eventdispatcher/utils.h"


// last include
//
#include    <snapdev/poison.h>




namespace ed
{



/** \brief Initializes the timer object.
 *
 * This function initializes the timer object with the specified \p timeout
 * defined in microseconds.
 *
 * Note that by default all connection objects are marked as persistent
 * since in most cases that is the type of connections you are interested
 * in. Therefore timers are also marked as persistent. This means if you
 * want a one time callback, you want to call the remove_connection()
 * function with your timer from your callback.
 *
 * \note
 * POSIX offers timers (in Linux since kernel version 2.6), only
 * (a) these generate signals, which is generally considered slow
 * in comparison to a timeout assigned to the poll() function, and
 * (b) the kernel posts at most one timer signal at a time across
 * one process, in other words, if 5 timers time out before you are
 * given a chance to process the timer, you only get one single
 * signal.
 *
 * \param[in] communicator  The communicator controlling this connection.
 * \param[in] timeout_us  The timeout in microseconds.
 */
timer::timer(std::int64_t timeout_us)
{
    if(timeout_us == 0)
    {
        // if zero, we assume that the timeout is a one time trigger
        // and that it will be set to other dates at other later times
        //
        set_timeout_date(get_current_date());
    }
    else
    {
        set_timeout_delay(timeout_us);
    }
}


/** \brief Retrieve the socket of the timer object.
*
* Timer objects are never attached to a socket so this function always
 * returns -1.
 *
 * \note
 * You should not override this function since there is not other
 * value it can return.
 *
 * \return Always -1.
 */
int timer::get_socket() const
{
    return -1;
}


/** \brief Tell that the socket is always valid.
 *
 * This function always returns true since the timer never uses a socket.
 *
 * \return Always true.
 */
bool timer::valid_socket() const
{
    return true;
}



} // namespace ed
// vim: ts=4 sw=4 et
