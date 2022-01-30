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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

/** \file
 * \brief Class used to manage message on a client connect on the server side.
 *
 * This class is used to create objects representing clients connecting
 * to a service.
 */

// self
//
#include    "eventdispatcher/connection_with_send_message.h"
#include    "eventdispatcher/dispatcher_support.h"
#include    "eventdispatcher/local_stream_server_client_buffer_connection.h"



namespace ed
{



class local_stream_server_client_message_connection
    : public local_stream_server_client_buffer_connection
    , public dispatcher_support
    , public connection_with_send_message
{
public:
    typedef std::shared_ptr<local_stream_server_client_message_connection>    pointer_t;

                                local_stream_server_client_message_connection(snapdev::raii_fd_t client);

    // connection_with_send_message implementation
    //
    virtual bool                send_message(message const & msg, bool cache = false) override;

    // local_stream_server_client_buffer_connection implementation
    //
    virtual void                process_line(std::string const & line) override;
};



} // namespace ed
// vim: ts=4 sw=4 et
