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
 * \brief Declaration of the remote snapcommunicator connection.
 *
 * This is the definition of the remote snapcommunicator connection.
 * Connection used to communicate with other snapcommunicators running
 * on other servers.
 */

// self
//
#include    "base_connection.h"


// eventdispatcher
//
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>



namespace sc
{



class remote_snapcommunicator
    : public ed::tcp_client_permanent_message_connection
    , public base_connection
{
public:
    static uint64_t const           REMOTE_CONNECTION_DEFAULT_TIMEOUT   =         1LL * 60LL * 1000000LL;   // 1 minute
    static uint64_t const           REMOTE_CONNECTION_RECONNECT_TIMEOUT =         5LL * 60LL * 1000000LL;   // 5 minutes
    static uint64_t const           REMOTE_CONNECTION_TOO_BUSY_TIMEOUT  = 24LL * 60LL * 60LL * 1000000LL;   // 24 hours

                                    remote_snapcommunicator(
                                              server::pointer_t cs
                                            , addr::addr const & addr);
    virtual                         ~remote_snapcommunicator() override;

    // tcp_client_permanent_message_connection implementation
    virtual void                    process_message(ed::message const & msg) override;
    virtual void                    process_connection_failed(std::string const & error_message) override;
    virtual void                    process_connected() override;

    addr::addr const &              get_address() const;

private:
    addr::addr                      f_address;
    int                             f_failures = -1;
    int64_t                         f_failure_start_time = 0;
    bool                            f_flagged = false;
    bool                            f_connected = false;
    std::string                     f_server_name = std::string();
};


} // namespace sc
// vim: ts=4 sw=4 et
