// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher/udp_server_connection.h"
#include    "eventdispatcher/dispatcher_support.h"



namespace ed
{



class udp_server_message_connection
    : public udp_server_connection
    , public dispatcher_support
{
public:
    typedef std::shared_ptr<udp_server_message_connection>    pointer_t;

    static size_t const         DATAGRAM_MAX_SIZE = 1024;

                                udp_server_message_connection(std::string const & addr, int port);

    static bool                 send_message(
                                          std::string const & addr
                                        , int port
                                        , message const & msg
                                        , std::string const & secret_code = std::string());

    // connection implementation
    virtual void                process_read() override;

private:
};



} // namespace ed
// vim: ts=4 sw=4 et
