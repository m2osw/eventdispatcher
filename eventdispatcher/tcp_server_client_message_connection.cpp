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
#include    "eventdispatcher/tcp_server_client_message_connection.h"

#include    "eventdispatcher/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// C++
//
#include    <cstring>


// C
//
#include    <arpa/inet.h>
#include    <sys/socket.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initializes a client to read messages from a socket.
 *
 * This implementation creates a message in/out client.
 * This is the most useful client in our Snap! Communicator
 * as it directly sends and receives messages.
 *
 * \param[in] client  The client representing the in/out socket.
 */
tcp_server_client_message_connection::tcp_server_client_message_connection(tcp_bio_client::pointer_t client)
    : tcp_server_client_buffer_connection(client)
{
    // TODO: somehow the port seems wrong (i.e. all connections return the same port)
    //       I changed this to do the getpeername() at the time you call the
    //       get_remote_address() and I use the addr::addr get_from_socket()
    //       so now it may work? I need to test it again

    // make sure the socket is defined and well
    //
    int const socket(client->get_socket());
    if(socket < 0)
    {
        SNAP_LOG_ERROR
            << "called with a closed client connection."
            << SNAP_LOG_SEND;
        throw std::runtime_error("tcp_server_client_message_connection() called with a closed client connection.");
    }
}


/** \brief Process a line (string) just received.
 *
 * The function parses the line as a message and then calls the
 * process_message() function if the line was valid.
 *
 * \param[in] line  The line of text that was just read.
 */
void tcp_server_client_message_connection::process_line(std::string const & line)
{
    // empty lines should not occur, but just in case, just ignore
    if(line.empty())
    {
        return;
    }

    message msg;
    if(msg.from_message(line))
    {
        dispatch_message(msg);
    }
    else
    {
        // TODO: what to do here? This could because the version changed
        //       and the messages are not compatible anymore.
        //
        SNAP_LOG_ERROR
            << "process_line() was asked to process an invalid message ("
            << line
            << ")"
            << SNAP_LOG_SEND;
    }
}


/** \brief Send a message.
 *
 * This function sends a message to the client on the other side
 * of this connection.
 *
 * \exception event_dispatcher_runtime_error
 * This function throws this exception if the write() to the pipe
 * fails to write the entire message. This should only happen if
 * the pipe gets severed.
 *
 * \param[in] msg  The message to be processed.
 * \param[in] cache  Whether to cache the message if there is no connection.
 *                   (Ignore because a client socket has to be there until
 *                   closed and then it can't be reopened by the server.)
 *
 * \return Always true. If an error occurs the function throws.
 */
bool tcp_server_client_message_connection::send_message(
          message & msg
        , bool cache)
{
    snapdev::NOT_USED(cache);

    // transform the message to a string and write to the socket
    // the writing is asynchronous so the message is saved in a cache
    // and transferred only later when the run() loop is hit again
    //
    std::string buf(msg.to_message());
    buf += '\n';
    return write(buf.c_str(), buf.length()) == static_cast<ssize_t>(buf.length());
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
 * \warning
 * This function returns BOTH: the address and the port.
 *
 * \note
 * These parameters are the same as what was passed to the constructor,
 * only both will have been converted to numbers. So for example when
 * you used "localhost", here you get "::1" or "127.0.0.1" for the
 * address.
 *
 * \return The remote host address and connection port.
 */
addr::addr tcp_server_client_message_connection::get_remote_address()
{
    if(f_remote_address.is_default())
    {
        int const s(get_socket());
        if(s >= 0)
        {
            f_remote_address.set_from_socket(s, true);
        }
    }

    return f_remote_address;
}


} // namespace ed
// vim: ts=4 sw=4 et
