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
 * \brief Appenders are used to append data to somewhere.
 *
 * This file declares the base appender class.
 */

// libaddr
//
#include    <libaddr/addr.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/connection_with_send_message.h>
#include    <eventdispatcher/dispatcher.h>
#include    <eventdispatcher/udp_server_message_connection.h>



namespace snaplogger_daemon
{


class udp_logger_server
    : public ed::udp_server_message_connection
{
public:
    typedef std::shared_ptr<udp_logger_server>
                                pointer_t;

                                udp_logger_server(
                                          addr::addr const & listen
                                        , std::string const & secret_code);
    virtual                     ~udp_logger_server() override;

    void                        msg_logger_message(ed::message & m);

private:
    ed::dispatcher::pointer_t   f_dispatcher = ed::dispatcher::pointer_t();
};


} // snaplogger_daemon namespace
// vim: ts=4 sw=4 et
