// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Implementation of the TCP server class.
 *
 * When instantiated, this class creates a TCP server. It creates a socket
 * and listen(2)'s on it for connection from clients. When such a
 * connection happens, the accept() function returns a socket with the
 * connection to the client and uses that to communicate with said client.
 */


// self
//
#include    "eventdispatcher/tcp_server_connection.h"


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initialize a server connection.
 *
 * This function is used to initialize a server connection, a TCP/IP
 * listener which can accept() new connections.
 *
 * The connection uses a \p mode parameter which can be set to MODE_PLAIN,
 * in which case the \p certificate and \p private_key parameters are
 * ignored, or MODE_SECURE.
 *
 * This connection supports secure SSL communication using a certificate
 * and a private key. These have to be specified as filenames. The
 * `communicatord` daemon makes use of files defined under
 * "/etc/communicator/ssl/..." by default.
 *
 * These files are created using this command line:
 *
 * \code
 * openssl req \
 *     -newkey rsa:2048 -nodes -keyout ssl-test.key \
 *     -x509 -days 3650 -out ssl-test.crt
 * \endcode
 *
 * Then pass "ssl-test.crt" as the certificate and "ssl-test.key"
 * as the private key.
 *
 * \todo
 * Add support for DH connections. Since our communicatord connections
 * are mostly private, it should not be a huge need at this point, though.
 *
 * \todo
 * Add support for verified certificates. Right now we do not create
 * signed certificates. This does not prevent fully secure transactions,
 * it just cannot verify that the computer on the other side is the
 * correct one.
 *
 * \param[in] address  The address and port to listen on. It may be set to "0.0.0.0" or "::".
 * \param[in] certificate  The filename to a .pem file.
 * \param[in] private_key  The filename to a .pem file.
 * \param[in] mode  The mode to use to open the connection (PLAIN or SECURE.)
 * \param[in] max_connections  The number of connections to keep in the listen queue.
 * \param[in] reuse_addr  Whether to mark the socket with the SO_REUSEADDR flag.
 */
tcp_server_connection::tcp_server_connection(
                  addr::addr const & address
                , std::string const & certificate
                , std::string const & private_key
                , mode_t mode
                , int max_connections
                , bool reuse_addr)
    : tcp_bio_server(
              address
            , max_connections
            , reuse_addr
            , certificate
            , private_key
            , mode)
{
}


/** \brief Reimplement the is_listener() for the tcp_server_connection.
 *
 * A server connection is a listener socket. The library makes
 * use of a completely different callback when a "read" event occurs
 * on these connections.
 *
 * The callback is expected to create the new connection and add
 * it the communicator.
 *
 * \return This version of the function always returns true.
 */
bool tcp_server_connection::is_listener() const
{
    return true;
}


/** \brief Retrieve the socket of this server connection.
 *
 * This function retrieves the socket this server connection. In this case
 * the socket is defined in the tcp_server class.
 *
 * \return The socket of this client connection.
 */
int tcp_server_connection::get_socket() const
{
    return tcp_bio_server::get_socket();
}



} // namespace ed
// vim: ts=4 sw=4 et
