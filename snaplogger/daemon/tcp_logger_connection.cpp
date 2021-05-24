// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Appenders are used to append data to somewhere.
 *
 * This file declares the base appender class.
 */

// self
//
#include    "snaplogger/daemon/tcp_logger_connection.h"

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


namespace
{


ed::dispatcher<tcp_logger_connection>::dispatcher_match::vector_t const g_messages =
{
    {
        "LOGGER"
      , &tcp_logger_connection::msg_logger_message
    },

    // ALWAYS LAST
    {
        nullptr
      , &tcp_logger_connection::msg_reply_with_unknown
      , &ed::dispatcher<tcp_logger_connection>::dispatcher_match::always_match
    }
};



}
// no name namespace



tcp_logger_connection::tcp_logger_connection(ed::tcp_bio_client::pointer_t client)
    : tcp_server_client_message_connection(client)
    , f_dispatcher(std::make_shared<ed::dispatcher<tcp_logger_connection>>(
              this
            , g_messages))
{
//#ifdef _DEBUG
//    f_dispatcher->set_trace();
//#endif
    set_dispatcher(f_dispatcher);
}


tcp_logger_connection::~tcp_logger_connection()
{
}


void tcp_logger_connection::msg_logger_message(ed::message & message)
{
    snaplogger::message::pointer_t msg(ed_message_to_log_message(message));
    msg->add_component(g_tcp_component);
}



} // snaplogger_daemon namespace
// vim: ts=4 sw=4 et
