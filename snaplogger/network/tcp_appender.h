// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
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
 * \brief The declaration of the TCP appender to send messages to a server.
 *
 * This file declares the TCP appender class which is used to send the logs
 * to a server listening on a TCP port.
 */

// self
//
#include    "base_network_appender.h"



// eventdispatcher lib
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/connection.h>





namespace snaplogger_network
{



class tcp_appender
    : public base_network_appender
{
public:
    typedef std::shared_ptr<tcp_appender>      pointer_t;

                                tcp_appender(std::string const & name);
    virtual                     ~tcp_appender() override;

    // appender implementation
    //
    virtual void                set_config(advgetopt::getopt const & params) override;

    // network_appender implementation
    //
    virtual void                server_address_changed() override;

protected:
    // implement appender
    //
    virtual void                process_message(
                                          snaplogger::message const & msg
                                        , std::string const & formatted_message) override;

private:
    ed::communicator::pointer_t f_communicator = ed::communicator::pointer_t();
    compression_t               f_compression = compression_t::COMPRESSION_NONE;
    bool                        f_fallback_to_console = false;
    ed::connection::pointer_t   f_connection = ed::connection::pointer_t();
};


} // snaplogger_network namespace
// vim: ts=4 sw=4 et
