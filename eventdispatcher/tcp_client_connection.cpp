// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher/tcp_client_connection.h"


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initializes the client connection.
 *
 * This function creates a connection using the address, port, and mode
 * parameters. This is very similar to using the bio_client class to
 * create a connection, only the resulting connection can be used with
 * the communicator object.
 *
 * \note
 * The function also saves the remote address and port used to open
 * the connection which can later be retrieved using the
 * get_remote_address() function. That address will remain valid
 * even after the socket is closed.
 *
 * \todo
 * If the remote address is an IPv6, we need to put it between [...]
 * (i.e. [::1]:4040) so we can extract the port safely.
 *
 * \param[in] communicator  The communicator controlling this connection.
 * \param[in] addr  The address of the server to connect to.
 * \param[in] port  The port to connect to.
 * \param[in] mode  Type of connection: plain or secure.
 */
tcp_client_connection::tcp_client_connection(std::string const & addr, int port, mode_t mode)
    : tcp_bio_client(addr, port, mode)
    , f_remote_address(get_client_addr() + ":" + std::to_string(get_client_port()))
{
}


/** \brief Retrieve the remote address information.
 *
 * This function can be used to retrieve the remove address and port
 * information as was specified on the constructor. These can be used
 * to find this specific connection at a later time or create another
 * connection.
 *
 * For example, you may get 192.168.2.17:4040.
 *
 * The function works even after the socket gets closed as we save
 * the remote address and port in a string just after the connection
 * was established.
 *
 * \note
 * These parameters are the same as what was passed to the constructor,
 * only both will have been converted to numbers. So for example when
 * you used "localhost", here you get "::1" or "127.0.0.1" for the
 * address.
 *
 * \return The remote host address and connection port.
 */
std::string const & tcp_client_connection::get_remote_address() const
{
    return f_remote_address;
}


/** \brief Read from the client socket.
 *
 * This function reads data from the client socket and copy it in
 * \p buf. A maximum of \p count bytes are read.
 *
 * \param[in,out] buf  The buffer where the data is read.
 * \param[in] count  The maximum number of bytes to read.
 *
 * \return -1 if an error occurs, zero if no data gets read, a positive
 *         number representing the number of bytes read otherwise.
 */
ssize_t tcp_client_connection::read(void * buf, size_t count)
{
    if(get_socket() == -1)
    {
        errno = EBADF;
        return -1;
    }
    return tcp_bio_client::read(reinterpret_cast<char *>(buf), count);
}


/** \brief Write to the client socket.
 *
 * This function writes \p count bytes from \p buf to this
 * client socket.
 *
 * The function can safely be called after the socket was closed, although
 * it will return -1 and set errno to EBADF in tha case.
 *
 * \param[in] buf  The buffer to write to the client connection socket.
 * \param[in] count  The maximum number of bytes to write on this connection.
 *
 * \return -1 if an error occurs, zero if nothing was written, a positive
 *         number representing the number of bytes successfully written.
 */
ssize_t tcp_client_connection::write(void const * buf, size_t count)
{
    if(get_socket() == -1)
    {
        errno = EBADF;
        return -1;
    }
    return tcp_bio_client::write(reinterpret_cast<char const *>(buf), count);
}


/** \brief Check whether this connection is a reader.
 *
 * We change the default to true since TCP sockets are generally
 * always readers. You can still overload this function and
 * return false if necessary.
 *
 * However, we do not overload the is_writer() because that is
 * much more dynamic (i.e. you do not want to advertise as
 * being a writer unless you have data to write to the
 * socket.)
 *
 * \return The events to listen to for this connection.
 */
bool tcp_client_connection::is_reader() const
{
    return true;
}


/** \brief Retrieve the socket of this client connection.
 *
 * This function retrieves the socket this client connection. In this case
 * the socket is defined in the bio_client class.
 *
 * \return The socket of this client connection.
 */
int tcp_client_connection::get_socket() const
{
    return tcp_bio_client::get_socket();
}



} // namespace ed
// vim: ts=4 sw=4 et
