// Snap Websites Server -- server to handle inter-process communication
// Copyright (c) 2011-2021  Made to Order Software Corp.  All Rights Reserved
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
#include    "version.h"


//// snapwebsites lib
////
//#include <snapwebsites/chownnm.h>
//#include <snapwebsites/flags.h>
//#include <snapwebsites/glob_dir.h>
//#include <snapwebsites/loadavg.h>
//#include <snapwebsites/log.h>
//#include <snapwebsites/qcompatibility.h>
//#include <snapwebsites/snap_communicator.h>
//#include <snapwebsites/snapwebsites.h>
//
//
//// snapdev lib
////
//#include <snapdev/not_used.h>
//#include <snapdev/tokenize_string.h>
//
//
//// libaddr lib
////
//#include <libaddr/addr_exception.h>
//#include <libaddr/addr_parser.h>
//#include <libaddr/iface.h>
//
//
//// Qt lib
////
//#include <QFile>
//
//
//// C++ lib
////
//#include <atomic>
//#include <cmath>
//#include <fstream>
//#include <iomanip>
//#include <sstream>
//#include <thread>
//
//
//// C lib
////
//#include <grp.h>
//#include <pwd.h>
//#include <sys/resource.h>









namespace sc
{



class remote_communicator_connections
        : public std::enable_shared_from_this<remote_communicator_connections>
{
public:
    typedef std::shared_ptr<remote_communicator_connections>    pointer_t;

                                            remote_communicator_connections(snap_communicator_server_pointer_t communicator, addr::addr const & my_addr);

    QString                                 get_my_address() const;
    void                                    add_remote_communicator(QString const & addr);
    void                                    stop_gossiping();
    void                                    too_busy(QString const & addr);
    void                                    shutting_down(QString const & addr);
    void                                    server_unreachable(QString const & addr);
    void                                    gossip_received(QString const & addr);
    void                                    forget_remote_connection(QString const & addr);
    tcp_client_server::bio_client::mode_t   connection_mode() const;
    size_t                                  count_live_connections() const;

private:
    snap_communicator_server_pointer_t      f_communicator_server = snap_communicator_server_pointer_t();
    addr::addr const &                      f_my_address;
    QMap<QString, int>                      f_all_ips = QMap<QString, int>();
    int64_t                                 f_last_start_date = 0;
    remote_snap_communicator_list_t         f_smaller_ips = remote_snap_communicator_list_t();      // we connect to smaller IPs
    gossip_snap_communicator_list_t         f_gossip_ips = gossip_snap_communicator_list_t();

    // larger IPs connect so they end up in the list with the local connections
    //service_connection_list_t               f_larger_ips = service_connection_list_t();       // larger IPs connect to us
};



} // sc namespace
// vim: ts=4 sw=4 et
