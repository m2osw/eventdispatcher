// Snap Websites Server -- server to handle inter-process communication
// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief Definition of the ping listener.
 *
 * The system can be pinged via a UDP connection.
 *
 * This is an addition to the normal TCP connection of the Snap!
 * Communicator when you want to send a quick message and you do not
 * need to wait for an answer.
 */

// self
//
#include    "server.h"


// eventdispatcher
//
#include    <eventdispatcher/udp_server_message_connection.h>



namespace sc
{



class ping
    : public ed::udp_server_message_connection
{
public:
    typedef std::shared_ptr<ping>  pointer_t;

                        ping(
                              server::pointer_t cs
                            , std::string const & addr);

    // ed::udp_server_message_connection implementation
    virtual void        process_message(ed::message const & message) override;

private:
    server::pointer_t   f_server = server::pointer_t();
};



} // sc namespace
// vim: ts=4 sw=4 et
