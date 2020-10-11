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
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// self
//
#include    "eventdispatcher/connection.h"



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
    typedef std::shared_ptr<timer>      pointer_t;

                                timer(std::int64_t timeout_us);

    // connection implementation
    virtual int                 get_socket() const override;
    virtual bool                valid_socket() const override;
};



} // namespace ed
// vim: ts=4 sw=4 et
