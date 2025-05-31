// Copyright (c) 2021-2025  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once

/** \file
 * \brief The declaration of the TCP logger server.
 *
 * This file declares the TCP logger server which listens for clients
 * to connect and send us LOG_MESSAGE messages.
 */

// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/tcp_server_connection.h>



namespace snaplogger_daemon
{


class tcp_logger_server
    : public ed::tcp_server_connection
{
public:
    typedef std::shared_ptr<tcp_logger_server>
                                pointer_t;

                                tcp_logger_server(addr::addr const & listen);
    virtual                     ~tcp_logger_server() override;

    // tcp_server_connection implementation
    //
    void                        process_accept();

private:
    ed::communicator::pointer_t f_communicator;
};


} // snaplogger_daemon namespace
// vim: ts=4 sw=4 et
