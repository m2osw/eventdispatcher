// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher/udp_server_message_connection.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/udp_client.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initialize a UDP server to send and receive messages.
 *
 * This function initializes a UDP server as a Snap UDP server
 * connection attached to the specified address and port.
 *
 * It is expected to be used to send and receive UDP messages.
 *
 * Note that to send messages, you need the address and port
 * of the destination. In effect, we do not use this server
 * when sending. Instead we create a client that we immediately
 * destruct once the message was sent.
 *
 * \param[in] addr  The address to listen on.
 * \param[in] port  The port to listen on.
 */
udp_server_message_connection::udp_server_message_connection(std::string const & addr, int port)
    : udp_server_connection(addr, port)
{
    // allow for looping over all the messages in one go
    //
    non_blocking();
}


/** \brief Send a UDP message.
 *
 * This function offers you to send a UDP message to the specified
 * address and port. The message should be small enough to fit in
 * one UDP packet or the call will fail.
 *
 * \note
 * The function returns true when the message was successfully sent.
 * This does not mean it was received.
 *
 * \param[in] addr  The destination address for the message.
 * \param[in] port  The destination port for the message.
 * \param[in] message  The message to send to the destination.
 * \param[in] secret_code  The secret code to send along the message.
 *
 * \return true when the message was sent, false otherwise.
 */
bool udp_server_message_connection::send_message(
                  std::string const & addr
                , int port
                , message const & msg
                , std::string const & secret_code)
{
    // Note: contrary to the TCP version, a UDP message does not
    //       need to include the '\n' character since it is sent
    //       in one UDP packet. However, it has a maximum size
    //       limit which we enforce here.
    //
    udp_client client(addr, port);
    std::string buf;
    if(!secret_code.empty())
    {
        message m(msg);
        m.add_parameter("udp_secret", secret_code);
        buf = m.to_message();
    }
    else
    {
        buf = msg.to_message();
    }

    // TODO: this maximum size needs to be checked dynamically
    //
    if(buf.length() > DATAGRAM_MAX_SIZE)
    {
        // packet too large for our buffers
        //
        throw event_dispatcher_invalid_message(
                  "message too large ("
                + std::to_string(buf.length())
                + " bytes) for a UDP server (max: "
                  BOOST_PP_STRINGIZE(DATAGRAM_MAX_SIZE));
    }

    if(client.send(buf.data(), buf.length()) != static_cast<ssize_t>(buf.length())) // we do not send the '\0'
    {
        // TODO: add errno to message
        SNAP_LOG_ERROR
            << "udp_server_message_connection::send_message(): could not send UDP message."
            << SNAP_LOG_SEND;
        return false;
    }

    return true;
}


/** \brief Implementation of the process_read() callback.
 *
 * This function reads the datagram we just received using the
 * recv() function. The size of the datagram cannot be more than
 * DATAGRAM_MAX_SIZE (1Kb at time of writing.)
 *
 * The message is then parsed and further processing is expected
 * to be accomplished in your implementation of process_message().
 *
 * The function actually reads as many pending datagrams as it can.
 */
void udp_server_message_connection::process_read()
{
    char buf[DATAGRAM_MAX_SIZE];
    for(;;)
    {
        ssize_t const r(recv(buf, sizeof(buf) / sizeof(buf[0])));
        if(r <= 0)
        {
            break;
        }
        std::string const udp_message(buf, r);
        message msg;
        if(msg.from_message(udp_message))
        {
            std::string const expected(get_secret_code());
            if(msg.has_parameter("udp_secret"))
            {
                std::string const secret(msg.get_parameter("udp_secret"));
                if(secret != expected)
                {
                    if(!expected.empty())
                    {
                        // our secret code and the message secret code do not match
                        //
                        SNAP_LOG_ERROR
                            << "the incoming message has an unexpected udp_secret code, message ignored."
                            << SNAP_LOG_SEND;
                        return;
                    }

                    // the sender included a UDP secret code but we don't
                    // require it so we emit a warning but still accept
                    // the message
                    //
                    SNAP_LOG_WARNING
                        << "no udp_secret=... parameter was expected (missing set_secret_code() call for this application?)"
                        << SNAP_LOG_SEND;
                }
            }
            else if(!expected.empty())
            {
                // secret code is missing from incoming message
                //
                SNAP_LOG_ERROR
                    << "the incoming message was expected to have udp_secret code, message ignored"
                    << SNAP_LOG_SEND;
                return;
            }

            // we received a valid message, process it
            //
            dispatch_message(msg);
        }
        else
        {
            SNAP_LOG_ERROR
                << "udp_server_message_connection::process_read() was asked"
                   " to process an invalid message ("
                << udp_message
                << ")"
                << SNAP_LOG_SEND;
        }
    }
}



} // namespace ed
// vim: ts=4 sw=4 et
