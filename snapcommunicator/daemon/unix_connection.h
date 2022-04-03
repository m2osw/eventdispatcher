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
 * \brief The declaration of the service class.
 *
 * The service class is the one used whenever a service connects to the
 * snapcommunicator daemon.
 */

// self
//
#include    "base_connection.h"
#include    "server.h"


// eventdispatcher
//
#include    <eventdispatcher/local_stream_server_client_message_connection.h>
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


class unix_connection
    : public ed::local_stream_server_client_message_connection
    , public base_connection
{
public:
    typedef std::shared_ptr<unix_connection>    pointer_t;

                        unix_connection(
                                  server::pointer_t cs
                                , snapdev::raii_fd_t client
                                , std::string const & server_name);
    virtual             ~unix_connection() override;

    // snap::snap_communicator::snap_tcp_server_client_message_connection implementation
    virtual void        process_message(ed::message const & msg) override;

    void                send_status();
    virtual void        process_timeout() override;
    virtual void        process_error() override;
    virtual void        process_hup() override;
    virtual void        process_invalid() override;
    void                properly_named();
    addr::addr const &  get_address() const;

private:
    std::string const   f_server_name;
    addr::addr          f_address = addr::addr();
    bool                f_named = false;
};



} // sc namespace
// vim: ts=4 sw=4 et
