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
 * \brief Declaration of the main server.
 *
 * The definition of the Snap! Communicator server. This class is the
 * one which drives everything else in the Snap! Communicator server.
 */

// self
//
#include    "cache.h"


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




class snap_communicator_server
    : public std::enable_shared_from_this<snap_communicator_server>
{
public:
    typedef std::shared_ptr<snap_communicator_server>     pointer_t;

    static std::size_t const    SNAP_COMMUNICATOR_MAX_CONNECTIONS = 100;

                                snap_communicator_server(snap::server::pointer_t s);
                                snap_communicator_server(snap_communicator_server const & src) = delete;
    snap_communicator_server &  operator = (snap_communicator_server const & rhs) = delete;

    void                        init();
    void                        run();

    // one place where all messages get processed
    void                        process_message(
                                          snap::snap_communicator::snap_connection::pointer_t connection
                                        , snap::snap_communicator_message const & message
                                        , bool udp);

    void                        send_status(
                                          snap::snap_communicator::snap_connection::pointer_t connection
                                        , snap::snap_communicator::snap_connection::pointer_t * reply_connection = nullptr);
    QString                     get_local_services() const;
    QString                     get_services_heard_of() const;
    void                        add_neighbors(QString const & new_neighbors);
    void                        remove_neighbor(QString const & neighbor);
    void                        read_neighbors();
    void                        save_neighbors();
    void                        verify_command(base_connection_pointer_t connection, snap::snap_communicator_message const & message);
    void                        process_connected(snap::snap_communicator::snap_connection::pointer_t connection);
    void                        broadcast_message(snap::snap_communicator_message const & message, base_connection_vector_t const & accepting_remote_connections = base_connection_vector_t());
    void                        process_load_balancing();
    tcp_client_server::bio_client::mode_t
                                connection_mode() const;
    void                        cluster_status(snap::snap_communicator::snap_connection::pointer_t reply_connection);
    void                        shutdown(bool quitting);

private:
    void                        drop_privileges();
    void                        refresh_heard_of();
    void                        listen_loadavg(snap::snap_communicator_message const & message);
    void                        save_loadavg(snap::snap_communicator_message const & message);
    void                        register_for_loadavg(QString const & ip);

    snap::server::pointer_t                             f_server = snap::server::pointer_t();

    QString                                             f_server_name = QString();
    int                                                 f_number_of_processors = 1;
    QString                                             f_neighbors_cache_filename = QString();
    QString                                             f_username = QString();
    QString                                             f_groupname = QString();
    std::string                                         f_public_ip = std::string();        // f_listener IP address
    snap::snap_communicator::pointer_t                  f_communicator = snap::snap_communicator::pointer_t();
    snap::snap_communicator::snap_connection::pointer_t f_interrupt = snap::snap_communicator::snap_connection::pointer_t();        // TCP/IP
    snap::snap_communicator::snap_connection::pointer_t f_local_listener = snap::snap_communicator::snap_connection::pointer_t();   // TCP/IP
    snap::snap_communicator::snap_connection::pointer_t f_listener = snap::snap_communicator::snap_connection::pointer_t();         // TCP/IP
    snap::snap_communicator::snap_connection::pointer_t f_ping = snap::snap_communicator::snap_connection::pointer_t();             // UDP/IP
    snap::snap_communicator::snap_connection::pointer_t f_loadavg_timer = snap::snap_communicator::snap_connection::pointer_t();    // a 1 second timer to calculate load (used to load balance)
    float                                               f_last_loadavg = 0.0f;
    addr::addr                                          f_my_address = addr::addr();
    QString                                             f_local_services = QString();
    sorted_list_of_strings_t                            f_local_services_list = sorted_list_of_strings_t();
    QString                                             f_services_heard_of = QString();
    sorted_list_of_strings_t                            f_services_heard_of_list = sorted_list_of_strings_t();
    QString                                             f_explicit_neighbors = QString();
    sorted_list_of_strings_t                            f_all_neighbors = sorted_list_of_strings_t();
    sorted_list_of_strings_t                            f_registered_neighbors_for_loadavg = sorted_list_of_strings_t();
    remote_communicator_connections::pointer_t          f_remote_snapcommunicators = remote_communicator_connections::pointer_t();
    size_t                                              f_max_connections = SNAP_COMMUNICATOR_MAX_CONNECTIONS;
    size_t                                              f_total_count_sent = 0; // f_all_neighbors.size() sent along CLUSTERUP/DOWN/COMPLETE/INCOMPLETE
    bool                                                f_shutdown = false;
    bool                                                f_debug_all_messages = false;
    bool                                                f_force_restart = false;
    message_cache::vector_t                             f_local_message_cache = message_cache::vector_t();
    std::map<QString, time_t>                           f_received_broadcast_messages = (std::map<QString, time_t>());
    tcp_client_server::bio_client::mode_t               f_connection_mode = tcp_client_server::bio_client::mode_t::MODE_PLAIN;
    QString                                             f_cluster_status = QString();
    QString                                             f_cluster_complete = QString();
};



} // sc namespace
// vim: ts=4 sw=4 et
