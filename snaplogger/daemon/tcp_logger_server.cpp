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
 * \brief Implementation of the TCP logger server.
 *
 * This file implements the TCP logger server which accepts connections from
 * all the other computers to accept logs from your entire network.
 */

// self
//
#include    "snaplogger/daemon/tcp_logger_server.h"

#include    "snaplogger/daemon/tcp_logger_connection.h"
#include    "snaplogger/daemon/network_component.h"



// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_daemon
{




tcp_logger_server::tcp_logger_server(addr::addr const & listen)
    : tcp_server_connection(
              listen.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY)
            , listen.get_port()
            , std::string()
            , std::string()
            , ed::tcp_bio_server::mode_t::MODE_PLAIN
            , -1
            , true)
    , f_communicator(ed::communicator::instance())
{
}


tcp_logger_server::~tcp_logger_server()
{
}


void tcp_logger_server::process_accept()
{
    ed::tcp_bio_client::pointer_t const new_client(accept());
    if(new_client == nullptr)
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << snaplogger::section(snaplogger::g_normal_component)
            << snaplogger::section(g_network_component)
            << snaplogger::section(g_daemon_component)
            << "accept() returned an error. (errno: "
            << e
            << " -- "
            << strerror(e)
            << "). No new connection will be created."
            << SNAP_LOG_SEND;
        return;
    }

    ed::connection::pointer_t client(std::make_shared<tcp_logger_connection>(new_client));
    client->set_name("client connection");

    if(!f_communicator->add_connection(client))
    {
        throw std::runtime_error("could not attach new client (tcp_logger_connection) to communicator");
    }
}




} // snaplogger_daemon namespace
// vim: ts=4 sw=4 et
