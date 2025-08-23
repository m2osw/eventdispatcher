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
 * \brief Timer connection.
 *
 * Class used to get a signal at a given date or every N seconds.
 *
 * The precision will depend on your hardware and kernel. The functions
 * support microseconds.
 *
 * The newer version supports adding callbacks meaning that you do not
 * need to create a new class to implement the process_timeout() function.
 */

// self
//
#include    <eventdispatcher/connection.h>


// snapdev
//
#include    <snapdev/callback_manager.h>



namespace ed
{



class timer
    : public connection
{
public:
    // timer is implemented using the timeout value on poll().
    // we could have another implementation that makes use of
    // the timerfd_create() function (in which case we'd be
    // limited to a date timeout, although an interval would
    // work too but require a little bit of work.)
    //
    typedef std::shared_ptr<timer>                          pointer_t;
    typedef std::function<bool(pointer_t timer_ptr)>        timeout_callback_t;
    typedef snapdev::callback_manager<timeout_callback_t>   callback_manager_t;

                                timer(std::int64_t timeout_us);

    callback_manager_t &        get_callback_manager();

    // connection implementation
    virtual int                 get_socket() const override;
    virtual bool                valid_socket() const override;
    virtual void                process_timeout() override;

private:
    callback_manager_t          f_callback_manager = callback_manager_t();
};



} // namespace ed
// vim: ts=4 sw=4 et
