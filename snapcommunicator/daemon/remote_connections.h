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
 * \brief Declaration of the remote connection.
 *
 * The remote connection allows this communicator to connect  to another
 * communicator on a remote computer.
 */

// self
//
#include    "server.h"



namespace sc
{



class remote_connections
    : public std::enable_shared_from_this<remote_connections>
{
public:
    typedef std::shared_ptr<remote_connections>    pointer_t;

                                            remote_connections(
                                                  server::pointer_t communicator
                                                , addr::addr const & my_addr);

    std::string                             get_my_address() const;
    void                                    add_remote_communicator(std::string const & addr);
    void                                    stop_gossiping();
    void                                    too_busy(std::string const & addr);
    void                                    shutting_down(std::string const & addr);
    void                                    server_unreachable(std::string const & addr);
    void                                    gossip_received(std::string const & addr);
    void                                    forget_remote_connection(std::string const & addr);
    tcp_client_server::bio_client::mode_t   connection_mode() const;
    size_t                                  count_live_connections() const;

private:
    server::pointer_t                       f_server = server::pointer_t();
    addr::addr const &                      f_my_address;
    int64_t                                 f_last_start_date = 0;
    sorted_list_of_addresses_t              f_all_ips = sorted_list_of_addresses_t();
    sorted_list_of_addresses_t              f_smaller_ips = sorted_list_of_addresses_t();   // we connect to smaller IPs
    sorted_list_of_addresses_t              f_gossip_ips = sorted_list_of_addresses_t();    // we gossip with larger IPs

    // larger IPs connect so they end up in the list with the local connections
    //service_connection_list_t               f_larger_ips = service_connection_list_t();       // larger IPs connect to us
};



} // sc namespace
// vim: ts=4 sw=4 et
