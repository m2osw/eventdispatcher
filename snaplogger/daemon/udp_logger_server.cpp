// Copyright (c) 2021-2022  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief UDP logger server to listen for log messages.
 *
 * This UDP server listens for LOGGER messages and sends them to
 * a local file. The UDP service can be used with one way messages
 * which may get lost along the way. Messages that make it will be
 * saved. Some message will receive an acknowledgement reply.
 */

// self
//
#include    "snaplogger/daemon/udp_logger_server.h"

#include    "snaplogger/daemon/network_component.h"
#include    "snaplogger/daemon/utils.h"


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_daemon
{



udp_logger_server::udp_logger_server(
          addr::addr const & listen
        , std::string const & secret_code)
    : udp_server_message_connection(listen)
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    set_secret_code(secret_code);
//#ifdef _DEBUG
//    f_dispatcher->set_trace();
//#endif
    set_dispatcher(f_dispatcher);

    f_dispatcher->add_matches({
        DISPATCHER_MATCH("LOGGER", &udp_logger_server::msg_logger_message),

        // ALWAYS LAST
        DISPATCHER_CATCH_ALL()
    });
}


udp_logger_server::~udp_logger_server()
{
}


void udp_logger_server::msg_logger_message(ed::message & m)
{
    snaplogger::message::pointer_t msg(ed_message_to_log_message(m));
    msg->add_component(g_udp_component);
    snaplogger::send_message(*msg);
}




} // snaplogger_daemon namespace
// vim: ts=4 sw=4 et
