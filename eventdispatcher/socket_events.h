// Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Send signal when a socket is opened in LISTEN mode.
 *
 * This class can be used to wait for a TCP socket to be opened and listening
 * for clients to connect. Later versions may offer more types of sockets
 * and more types of events.
 */

// self
//
#include    "eventdispatcher/connection.h"


// libaddr lib
//
#include    <libaddr/addr.h>



namespace ed
{



class socket_events
    : public connection
{
public:
    typedef std::shared_ptr<socket_events>  pointer_t;

    static std::int64_t const   DEFAULT_PAUSE_BETWEEN_POLLS = 10LL;  // 10 seconds

                                socket_events(addr::addr const & a);
                                socket_events(
                                          std::string const & address
                                        , int port);
    virtual                     ~socket_events() override;

    // connection implementation
    virtual int                 get_socket() const override;

    // new callbacks
    virtual void                process_listening() = 0;

    addr::addr const &          get_addr() const;
    void                        lost_connection();

private:
    addr::addr                  f_addr = addr::addr();
};



} // namespace ed
// vim: ts=4 sw=4 et
