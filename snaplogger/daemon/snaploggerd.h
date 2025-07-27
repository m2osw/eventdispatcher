// Copyright (c) 2021-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

/** \file
 * \brief Appenders are used to append data to somewhere.
 *
 * This file declares the base appender class.
 */

// self
//
#include    "snaplogger/daemon/tcp_logger_server.h"
#include    "snaplogger/daemon/udp_logger_server.h"



// eventdispatcher
//
#include    <eventdispatcher/logrotate_udp_messenger.h>


// advgetopt
//
#include    <advgetopt/advgetopt.h>




namespace snaplogger_daemon
{


constexpr int                       DEFAULT_CONTROLLER_PORT = 4050;
constexpr int                       DEFAULT_LOGROTATE_PORT  = 4051;
constexpr int                       DEFAULT_UDP_PORT        = 4052;
constexpr int                       DEFAULT_TCP_PORT        = 4053;


class snaploggerd
    : public std::enable_shared_from_this<snaploggerd>
{
public:
    typedef std::shared_ptr<snaploggerd>
                                    pointer_t;

                                    snaploggerd(int argc, char * argv[]);

    bool                            init();
    int                             run();

private:
    advgetopt::getopt               f_opts;
    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    ed::logrotate_extension         f_logrotate;
    tcp_logger_server::pointer_t    f_tcp_server = tcp_logger_server::pointer_t();
    udp_logger_server::pointer_t    f_udp_server = udp_logger_server::pointer_t();
};


} // snaplogger_daemon namespace
// vim: ts=4 sw=4 et
