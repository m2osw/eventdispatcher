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
#include    "utils.h"


// eventdispatcher
//
#include    <eventdispatcher/connection.h>
#include    <eventdispatcher/communicator.h>
//#include <snapwebsites/flags.h>
//#include <snapwebsites/glob_dir.h>
//#include <snapwebsites/loadavg.h>
//#include <snapwebsites/log.h>
//#include <snapwebsites/qcompatibility.h>
//#include <snapwebsites/snap_communicator.h>
//#include <snapwebsites/snapwebsites.h>


//// snapdev lib
////
//#include <snapdev/not_used.h>
//#include <snapdev/tokenize_string.h>


// libaddr lib
//
#include    <libaddr/addr.h>
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


class base_connection;
class remote_connections;


class server
    : public std::enable_shared_from_this<server>
{
public:
    typedef std::shared_ptr<server>     pointer_t;

    static std::size_t const    SNAP_COMMUNICATOR_MAX_CONNECTIONS = 100;

                                server(int argc, char * argv[]);
                                server(server const & src) = delete;
    server &                    operator = (server const & rhs) = delete;

    int                         run();

    // one place where all messages get processed
    void                        process_message(
                                          ed::connection::pointer_t connection
                                        , ed::message const & message
                                        , bool udp);

    void                        send_status(
                                          ed::connection::pointer_t connection
                                        , ed::connection::pointer_t * reply_connection = nullptr);
    std::string                 get_local_services() const;
    std::string                 get_services_heard_of() const;
    void                        add_neighbors(std::string const & new_neighbors);
    void                        remove_neighbor(std::string const & neighbor);
    void                        read_neighbors();
    void                        save_neighbors();
    void                        verify_command(
                                          std::shared_ptr<base_connection> connection
                                        , ed::message const & message);
    void                        process_connected(ed::connection::pointer_t connection);
    void                        broadcast_message(
                                          ed::message const & message
                                        , std::vector<base_connection> const & accepting_remote_connections = std::vector<base_connection>());
    void                        process_load_balancing();
    void                        cluster_status(ed::connection::pointer_t reply_connection);
    void                        shutdown(bool quitting);

private:
    int                         init();
    void                        drop_privileges();
    void                        refresh_heard_of();
    void                        listen_loadavg(ed::message const & message);
    void                        save_loadavg(ed::message const & message);
    void                        register_for_loadavg(std::string const & ip);

    //snap::server::pointer_t   f_server = snap::server::pointer_t(); -- this was the snapwebsites server

    std::string                     f_server_name = std::string();
    int                             f_number_of_processors = 1;
    std::string                     f_neighbors_cache_filename = std::string();
    std::string                     f_username = std::string();
    std::string                     f_groupname = std::string();
    std::string                     f_public_ip = std::string();        // f_listener IP address
    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    ed::connection::pointer_t       f_interrupt = ed::connection::pointer_t();        // TCP/IP
    ed::connection::pointer_t       f_local_listener = ed::connection::pointer_t();   // TCP/IP
    ed::connection::pointer_t       f_remote_listener = ed::connection::pointer_t();  // TCP/IP
    ed::connection::pointer_t       f_secure_listener = ed::connection::pointer_t();  // TCP/IP
    ed::connection::pointer_t       f_unix_listener = ed::connection::pointer_t();    // Unix socket
    ed::connection::pointer_t       f_ping = ed::connection::pointer_t();             // UDP/IP
    ed::connection::pointer_t       f_loadavg_timer = ed::connection::pointer_t();    // a 1 second timer to calculate load (used to load balance)
    float                           f_last_loadavg = 0.0f;
    addr::addr                      f_my_address = addr::addr();
    std::string                     f_local_services = std::string();
    sorted_list_of_strings_t        f_local_services_list = sorted_list_of_strings_t();
    std::string                     f_services_heard_of = std::string();
    sorted_list_of_strings_t        f_services_heard_of_list = sorted_list_of_strings_t();
    std::string                     f_explicit_neighbors = std::string();
    sorted_list_of_strings_t        f_all_neighbors = sorted_list_of_strings_t();
    sorted_list_of_strings_t        f_registered_neighbors_for_loadavg = sorted_list_of_strings_t();
    std::shared_ptr<remote_connections>
                                    f_remote_snapcommunicators = std::shared_ptr<remote_connections>();
    size_t                          f_max_connections = SNAP_COMMUNICATOR_MAX_CONNECTIONS;
    size_t                          f_total_count_sent = 0; // f_all_neighbors.size() sent along CLUSTERUP/DOWN/COMPLETE/INCOMPLETE
    bool                            f_shutdown = false;
    bool                            f_debug_all_messages = false;
    bool                            f_force_restart = false;
    cache                           f_local_message_cache = cache();
    std::map<std::string, time_t>   f_received_broadcast_messages = (std::map<std::string, time_t>());
    std::string                     f_cluster_status = std::string();
    std::string                     f_cluster_complete = std::string();
};



} // namespace sc
// vim: ts=4 sw=4 et
