// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// self
//
#include    <eventdispatcher/connection_with_send_message.h>
#include    <eventdispatcher/local_dgram_server_connection.h>
#include    <eventdispatcher/local_dgram_client.h>
#include    <eventdispatcher/dispatcher_support.h>



namespace ed
{



class local_dgram_server_message_connection
    : public local_dgram_server_connection
    , public dispatcher_support
    , public connection_with_send_message
{
public:
    typedef std::shared_ptr<local_dgram_server_message_connection>    pointer_t;

    static size_t const         DATAGRAM_MAX_SIZE = 64 * 1024;

                                local_dgram_server_message_connection(
                                          addr::addr_unix const & address
                                        , bool sequential
                                        , bool close_on_exec
                                        , bool force_reuse_addr
                                        , addr::addr_unix const & client_address = addr::addr_unix()
                                        , std::string const & service_name = std::string());

    bool                        send_message(
                                          message const & msg
                                        , std::string const & secret_code = std::string());

    static bool                 send_message(
                                          addr::addr_unix const & address
                                        , message & msg
                                        , std::string const & secret_code = std::string());

    static bool                 send_message(
                                          local_dgram_client & client
                                        , message const & msg
                                        , std::string const & secret_code = std::string());

    // connection implementation
    //
    virtual void                process_read() override;

    // connection_with_send_message implementation
    virtual bool                send_message(
                                          message & msg
                                        , bool cache = false) override;

private:
    local_dgram_client::pointer_t
                                f_dgram_client = local_dgram_client::pointer_t();
};



} // namespace ed
// vim: ts=4 sw=4 et
