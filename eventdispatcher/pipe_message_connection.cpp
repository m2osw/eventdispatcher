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
#include    "eventdispatcher/pipe_message_connection.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// last include
//
#include    <snapdev/poison.h>




namespace ed
{



/** \brief Send a message.
 *
 * This function sends a message to the process on the other side
 * of this pipe connection.
 *
 * \exception event_dispatcher_runtime_error
 * This function throws this exception if the write() to the pipe
 * fails to write the entire message. This should only happen if
 * the pipe gets severed.
 *
 * \param[in] msg  The message to be sent.
 * \param[in] cache  Whether to cache the message if there is no connection.
 *                   (Ignore because a pipe has to always be there until
 *                   closed and then it can't be reopened.)
 *
 * \return Always true, although if an error occurs the function throws.
 */
bool pipe_message_connection::send_message(message & msg, bool cache)
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


/** \brief Process a line (string) just received.
 *
 * The function parses the line as a message and then calls the
 * process_message() function if the line was valid.
 *
 * \param[in] line  The line of text that was just read.
 */
void pipe_message_connection::process_line(std::string const & line)
{
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
        // TODO: what to do here? This could be that the version changed
        //       and the messages are not compatible anymore.
        //
        SNAP_LOG_ERROR
            << "pipe_message_connection::process_line() was asked to process an invalid message ("
            << line
            << ")"
            << SNAP_LOG_SEND;
    }
}



} // namespace ed
// vim: ts=4 sw=4 et
