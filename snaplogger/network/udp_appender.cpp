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

/** \file
 * \brief Implementation of the UDP appender.
 *
 * This file implements the necessary to send UDP messages to the
 * snaplogger daemon.
 */

// self
//
#include    "snaplogger/network/udp_appender.h"


// snaplogger
//
#include    <snaplogger/guard.h>


// eventdispatcher
//
#include    <eventdispatcher/udp_server_message_connection.h>


// C++
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_network
{


namespace
{


APPENDER_FACTORY(udp);


}
// no name namespace



udp_appender::udp_appender(std::string const & name)
    : base_network_appender(name, "udp")
{
}


udp_appender::~udp_appender()
{
}


void udp_appender::set_config(advgetopt::getopt const & opts)
{
    base_network_appender::set_config(opts);

    // SECRET CODE
    //
    std::string const secret_code_field(get_name() + "::secret_code");
    if(opts.is_defined(secret_code_field))
    {
        f_secret_code = opts.get_string(secret_code_field);
    }
}


void udp_appender::process_message(snaplogger::message const & msg, std::string const & formatted_message)
{
    snaplogger::guard g;

    ed::message log_message;
    log_message_to_ed_message(msg, log_message);

    // for UDP we also have an "acknowledge" parameter
    //
    if(f_acknowledge == acknowledge_t::ACKNOWLEDGE_SEVERITY
    && msg.get_severity() >= f_acknowledge_severity)
    {
        // TODO: put a return address:port
        //
        log_message.add_parameter("acknowledge", "true");

        // TODO: we need to save messages and wait for the acknowledgement
        //       if not acknowledge in `timeout`, then try to resend it
        //
        //f_waiting_acknowledgement.push_back(log_message);
        // TODO: set a timeout & # of retries...
    }

    // send message via UDP
    //
    bool const success(ed::udp_server_message_connection::send_message(
                  f_server_address
                , log_message
                , f_secret_code));
    if(!success)
    {
        // how could we report that? we are the logger...
        if(f_fallback_to_console
        && isatty(fileno(stdout)))
        {
            std::cout << formatted_message.c_str();
        }
    }
}





} // snaplogger namespace
// vim: ts=4 sw=4 et
