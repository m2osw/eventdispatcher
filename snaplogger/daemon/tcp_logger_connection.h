// Copyright (c) 2021-2023  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once

/** \file
 * \brief Appenders are used to append data to somewhere.
 *
 * This file declares the base appender class.
 */

// eventdispatcher
//
#include    <eventdispatcher/dispatcher.h>
#include    <eventdispatcher/tcp_server_client_message_connection.h>



namespace snaplogger_daemon
{


class tcp_logger_connection
    : public ed::tcp_server_client_message_connection
{
public:
    typedef std::shared_ptr<tcp_logger_connection>
                                pointer_t;

                                tcp_logger_connection(ed::tcp_bio_client::pointer_t client);
    virtual                     ~tcp_logger_connection() override;

    void                        msg_logger_message(ed::message & m);

private:
    ed::dispatcher::pointer_t   f_dispatcher = ed::dispatcher::pointer_t();
};


} // snaplogger_daemon namespace
// vim: ts=4 sw=4 et
