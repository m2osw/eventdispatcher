// Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Listen for UDP messages.
 *
 * This class is used to create an object ready to listen for incoming
 * messages. It also supports a way to send messages either through the
 * send() function or setting the client/server flag to true and have
 * a client and a server within this class.
 */

// self
//
#include    <eventdispatcher/connection_with_send_message.h>
#include    <eventdispatcher/dispatcher_support.h>
#include    <eventdispatcher/udp_client.h>
#include    <eventdispatcher/udp_server_connection.h>



namespace ed
{



class udp_server_message_connection
    : public udp_server_connection
    , public dispatcher_support
    , public connection_with_send_message
{
public:
    typedef std::shared_ptr<udp_server_message_connection>    pointer_t;

    static size_t const         DATAGRAM_MAX_SIZE = 1024;

                                udp_server_message_connection(
                                          addr::addr const & server_address
                                        , addr::addr const & client_address = addr::addr());

    virtual bool                send_message(
                                          message & msg
                                        , bool cache = false);

    bool                        send_message(
                                          message const & msg
                                        , std::string const & secret_code = std::string());

    static bool                 send_message(
                                          addr::addr const & client_address
                                        , message const & msg
                                        , std::string const & secret_code = std::string());

    static bool                 send_message(
                                          udp_client & client
                                        , message const & msg
                                        , std::string const & secret_code = std::string());

    // connection implementation
    virtual void                process_read() override;

    void                        set_secret_code(std::string const & secret_code);
    std::string                 get_secret_code() const;

private:
    udp_client::pointer_t       f_udp_client = udp_client::pointer_t();
    std::string                 f_secret_code = std::string();
};



} // namespace ed
// vim: ts=4 sw=4 et
