// Copyright (c) 2020 Made to Order Software Corp.
// All rights reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// self
//
#include    "eventdispatcher/logrotate_udp_messenger.h"


// last include
//
#include    <snapdev/poison.h>




/** \brief The UDP server initialization.
 *
 * The UDP server creates a new UDP server to listen on incoming
 * messages over a UDP connection.
 *
 * This UDP implementation is used to listen to the LOG message specifically.
 * It is often that you want to restart the snaplogger and this can be used
 * as the way to do it. The UDP message can be sent using the ed-signal
 * command line tool:
 *
 * \code
 *     ed-signal --server 127.0.0.1:1234 --message LOG --type udp
 * \endcode
 *
 * The reaction of the server is to restart the snaplogger as implemented in
 * the default dispatcher messages (see the messages.cpp for details).
 *
 * If you already have a UDP service in your application, you can simply
 * add the default dispatcher messages and the LOG message will automatically
 * be handled for you. No need to create yet another network connection.
 * We do so in this constructor like so:
 *
 * \code
 *     f_dispatcher->add_communicator_commands();
 * \endcode
 *
 * \param[in] addr  The address/port to listen on. Most often it is the private
 * address (127.0.0.1).
 * \param[in] secret_code  A secret code to verify in order to accept UDP
 * messages.
 */
logrotate_udp_messenger::logrotate_udp_messenger(
          addr::addr const & address
        , std::string const & secret_code)
    : ed::udp_server_message_connection(address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS), address.get_port())
    , f_dispatcher(std::make_shared<ed::dispatcher<logrotate_udp_messenger>>(
                          this
                        , ed::dispatcher<logrotate_udp_messenger>::dispatcher_match::vector_t()))
{
    set_secret_code(secret_code);
    f_dispatcher->add_communicator_commands();
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);
}


logrotate_udp_messenger::~logrotate_udp_messenger()
{
}


// vim: ts=4 sw=4 et
