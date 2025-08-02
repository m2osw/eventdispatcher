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

/** \file
 * \brief Implementation of the Snap Communicator class.
 *
 * This class wraps the C poll() interface in a C++ object with many types
 * of objects:
 *
 * \li Server Connections; for software that want to offer a port to
 *     which clients can connect to; the server will call accept()
 *     once a new client connection is ready; this results in a
 *     Server/Client connection object
 * \li Client Connections; for software that want to connect to
 *     a server; these expect the IP address and port to connect to
 * \li Server/Client Connections; for the server when it accepts a new
 *     connection; in this case the server gets a socket from accept()
 *     and creates one of these objects to handle the connection
 *
 * Using the poll() function is the easiest and allows us to listen
 * on pretty much any number of sockets (on my server it is limited
 * at 16,768 and frankly over 1,000 we probably will start to have
 * real slowness issues on small VPN servers.)
 */


// self
//
#include    "eventdispatcher/udp_server_connection.h"


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initialize a UDP listener.
 *
 * This function is used to initialize a server connection, a UDP/IP
 * listener which wakes up whenever a send() is sent to this listener
 * address and port.
 *
 * \param[in] address  The address and port to listen on. The address
 * can be the default address.
 * \param[in] multicast_address  A multicast address (224.x.x.x) or the
 * default address.
 */
udp_server_connection::udp_server_connection(
              addr::addr const & address
            , addr::addr const & multicast_address)
    : udp_server(address, multicast_address)
{
}


/** \brief Check to know whether this UDP connection is a reader.
 *
 * This function returns true to say that this UDP connection is
 * indeed a reader.
 *
 * \return This function already returns true as we are likely to
 *         always want a UDP socket to be listening for incoming
 *         packets.
 */
bool udp_server_connection::is_reader() const
{
    return true;
}


/** \brief Retrieve the socket of this server connection.
 *
 * This function retrieves the socket this server connection. In this case
 * the socket is defined in the udp_server class.
 *
 * \return The socket of this client connection.
 */
int udp_server_connection::get_socket() const
{
    return udp_server::get_socket();
}


/** \brief Define a secret code.
 *
 * When receiving a message through this UDP socket, this secret code must
 * be included in the message. If not present, then the message gets
 * discarded.
 *
 * By default this parameter is an empty string. This means no secret
 * code is required and UDP communication can be done without it.
 *
 * \note
 * Secret codes are expected to be used only on connections between
 * computers. If the IP address is 127.0.0.1, you probably don't need
 * to have a secret code.
 *
 * \warning
 * Remember that UDP messages are limited in size. If too long, the
 * send_message() function throws an error. So your secret code should
 * remain relatively small.
 *
 * \todo
 * The secret_code string must be a valid UTF-8 string. At this point
 * this is not enforced.
 *
 * \param[in] secret_code  The secret code that has to be included in the
 * incoming messages for those to be accepted.
 */
void udp_server_connection::set_secret_code(std::string const & secret_code)
{
    f_secret_code = secret_code;
}


/** \brief Retrieve the server secret code.
 *
 * This function returns the server secret code as defined with the
 * set_secret_code() function. By default this parameter is set to
 * the empty string.
 *
 * Whenever a message is received, this code is checked. If defined
 * in the server and not equal to the code in the message, then the
 * message is discarded (hackers?)
 *
 * The message is also used when sending a message. It gets added
 * to the message if it is not the empty string.
 *
 * \return The secret code.
 */
std::string const & udp_server_connection::get_secret_code() const
{
    return f_secret_code;
}



} // namespace ed
// vim: ts=4 sw=4 et
