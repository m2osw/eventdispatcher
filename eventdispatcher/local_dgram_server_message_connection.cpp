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
 * \brief Implementation of the AF_UNIX socket class handling message packets.
 *
 */

// self
//
#include    "eventdispatcher/local_dgram_server_message_connection.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/local_dgram_client.h"


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
 * \param[in] address  The address to listen on. It may be set to "0.0.0.0".
 * \param[in] sequential  Whether the packets are kept in order.
 * \param[in] close_on_exec  Whether the socket is closed on execve().
 * \param[in] force_reuse_addr  Whether caller is okay with an unlink() of
 * a file socket if it exists.
 */
local_dgram_server_message_connection::local_dgram_server_message_connection(
              addr::unix const & address
            , bool sequential
            , bool close_on_exec
            , bool force_reuse_addr)
    : local_dgram_server_connection(
          address
        , sequential
        , close_on_exec
        , force_reuse_addr)
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
 * \param[in] address  The destination Unix address for the message.
 * \param[in] msg  The message to send to the destination.
 * \param[in] secret_code  The secret code to send along the message.
 *
 * \return true when the message was sent, false otherwise.
 */
bool local_dgram_server_message_connection::send_message(
                  addr::unix const & address
                , message const & msg
                , std::string const & secret_code)
{
    // Note: contrary to the Stream version, a Datagram message does not
    //       need to include the '\n' character since it is sent
    //       in one packet. However, it has a maximum size limit
    //       which we enforce here.
    //
    std::string buf;
    if(!secret_code.empty())
    {
        message m(msg);
        m.add_parameter("secret_code", secret_code);
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
        // packet too large for the socket buffers
        //
        throw event_dispatcher_invalid_message(
                  "message too large ("
                + std::to_string(buf.length())
                + " bytes) for a Unix socket (max: "
                  BOOST_PP_STRINGIZE(DATAGRAM_MAX_SIZE));
    }

    local_dgram_client client(address);
    int const r(client.send(buf.data(), buf.length()));
    if(r != static_cast<ssize_t>(buf.length())) // we do not send the '\0'
    {
        // TODO: add errno to message
        int const e(errno);
        SNAP_LOG_ERROR
            << "local_dgram_server_message_connection::send_message(): could not send Datagram message to \""
            << address.to_uri()
            << "\", errno: "
            << e
            << ", "
            << strerror(e)
            << " (r = "
            << r
            << ")."
            << SNAP_LOG_SEND;
        errno = e;
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
void local_dgram_server_message_connection::process_read()
{
    char buf[DATAGRAM_MAX_SIZE];
    for(;;)
    {
        ssize_t const r(recv(buf, sizeof(buf) / sizeof(buf[0])));
        if(r <= 0)
        {
            break;
        }
        std::string const local_dgram_message(buf, r);
        message msg;
        if(msg.from_message(local_dgram_message))
        {
            std::string const expected(get_secret_code());
            if(msg.has_parameter("secret_code"))
            {
                std::string const secret(msg.get_parameter("secret_code"));
                if(secret != expected)
                {
                    if(!expected.empty())
                    {
                        // our secret code and the message secret code do not match
                        //
                        SNAP_LOG_ERROR
                            << "the incoming message has an unexpected secret_code code, message ignored."
                            << SNAP_LOG_SEND;
                        return;
                    }

                    // the sender included a UDP secret code but we don't
                    // require it so we emit a warning but still accept
                    // the message
                    //
                    SNAP_LOG_WARNING
                        << "no secret_code=... parameter was expected (missing set_secret_code() call for this application?)."
                        << SNAP_LOG_SEND;
                }
            }
            else if(!expected.empty())
            {
                // secret code is missing from incoming message
                //
                SNAP_LOG_ERROR
                    << "the incoming message was expected to have a secret_code parameter, message dropped."
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
                << "local_dgram_server_message_connection::process_read() was"
                   " asked to process an invalid message ("
                << local_dgram_message
                << ")"
                << SNAP_LOG_SEND;
        }
    }
}



} // namespace ed
// vim: ts=4 sw=4 et
