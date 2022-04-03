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
 * \brief Definition of the Gossip connection.
 *
 * The Gossip connection is used to let another communicator know about
 * us when it is expected that this other communicator connect to us
 * (based of our respective IP addresses).
 */

// self
//
#include    "server.h"


// eventdispatcher
//
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>
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


class gossip_to_remote_snap_communicator
    : public ed::tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<gossip_to_remote_snap_communicator> pointer_t;

    static int64_t const        FIRST_TIMEOUT = 5LL * 1000000L;  // 5 seconds before first attempt

                                gossip_to_remote_snap_communicator(
                                          remote_communicator_connections::pointer_t rcs
                                        , std::string const & addr
                                        , int port);

    // connection implementation
    virtual void                process_timeout();

    // tcp_client_permanent_message_connection implementation
    virtual void                process_message(ed::message const & message) override;
    virtual void                process_connection_failed(std::string const & error_message) override;
    virtual void                process_connected() override;

    void                        kill();

private:
    std::string const           f_addr;
    int const                   f_port = 0;
    int64_t                     f_wait = FIRST_TIMEOUT;
    remote_connections::pointer_t
                                f_remote_communicators = remote_connections::pointer_t();
};


} // sc namespace
// vim: ts=4 sw=4 et
