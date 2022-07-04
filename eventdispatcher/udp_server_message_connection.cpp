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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Implementation of the UDP server class.
 *
 * This class is used to listen and optionally send UDP messages.
 *
 * By default, the class only creates a server. With UDP, since it is state
 * less, the only way to communicate is via two servers and two clients.
 * A client is used to send messages and a server is used to listen and
 * receive messages.
 *
 * Since the port for the server and the client need to be different.
 * You may assign the server port 0 in which case it is automatically
 * generated and that port can be sent to the other side so that other
 * side can then reply to our messages.
 */

// self
//
#include    "eventdispatcher/udp_server_message_connection.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/udp_client.h"


// snaplogger
//
#include    <snaplogger/message.h>


// libaddr
//
#include    <libaddr/iface.h>


// boost
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
 * The \p client_address, if not set to ANY (0.0.0.0 or ::) is
 * used to create a udp_client object. That object is used
 * by the send_message() function. It also allows you to use
 * port 0 for the server which means you do not have to have
 * a reserved port for the server. That port can then be sent
 * to the client which can use it to send you replies.
 *
 * \param[in] server_address  The address and port to listen on.
 * \param[in] client_address  The address for the client side.
 */
udp_server_message_connection::udp_server_message_connection(
          addr::addr const & server_address
        , addr::addr const & client_address)
    : udp_server_connection(server_address)
{
    // allow for looping over all the messages in one go
    //
    non_blocking();

    if(client_address.get_network_type() != addr::network_type_t::NETWORK_TYPE_ANY)
    {
        f_udp_client = std::make_shared<ed::udp_client>(client_address);
    }
}


/** \brief Send a message.
 *
 * This function sends \p message to the other side.
 *
 * The \p cache parameter is here because it is present in the send_message()
 * of the connection_with_send_message class. It is not used by the UDP
 * implementation, however.
 *
 * \param[in,out] msg  The message to forward to the other side.
 * \param[in] cache  This flag is ignored.
 *
 * \return true if the message was sent successfully.
 */
bool udp_server_message_connection::send_message(
          message & msg
        , bool cache)
{
    snapdev::NOT_USED(cache);

    return send_message(msg, f_secret_code);
}


/** \brief Send a message over to the client.
 *
 * This function sends a message to the client at the address specified in
 * the constructor.
 *
 * The advantage of using this function is that the server port is
 * automatically attached to the message through the reply_port
 * parameter. This is important if you are running an application
 * which is not itself the main server (since the UDP mechanism is
 * opposite to the TCP mechanism, clients have to create servers
 * which have to listen and on one computer, multiple clients
 * would require you to assign additional ports to clients, which
 * is unusual).
 *
 * \exception initialization_missing
 * If no address was specified on the constructor (i.e. the ANY address
 * was used) then this exception is raised.
 *
 * \param[in] msg  The message to send to the client.
 * \param[in] secret_code  The secret code to attach to the message.
 *
 * \return true when the message was sent, false otherwise.
 */
bool udp_server_message_connection::send_message(
          message const & msg
        , std::string const & secret_code)
{
    if(f_udp_client == nullptr)
    {
        throw initialization_missing("this UDP server was not initialized with a client (see constructor).");
    }

    message with_address(msg);
    with_address.add_parameter("reply_to", get_address());

    return send_message(*f_udp_client, msg, secret_code);
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
 * \param[in] client_address  The destination address and port for the message.
 * \param[in] msg  The message to send to the destination.
 * \param[in] secret_code  The secret code to send along the message.
 *
 * \return true when the message was sent, false otherwise.
 */
bool udp_server_message_connection::send_message(
          addr::addr const & client_address
        , message const & msg
        , std::string const & secret_code)
{
    // Note: contrary to the TCP version, a UDP message does not
    //       need to include the '\n' character since it is sent
    //       in one UDP packet. However, it has a maximum size
    //       limit which we enforce here.
    //
    udp_client client(client_address);

    // you should use the multi-cast
    //
    // TODO: also the is_broadcast_address() re-reads the list of interfaces
    //       from the kernel, which is _slow_ (i.e. it doesn't get cached)
    //       See: libaddr/iface.cpp in the libaddr project
    //
    if(client_address.get_network_type() == addr::network_type_t::NETWORK_TYPE_MULTICAST
    || addr::is_broadcast_address(client_address))
    {
        client.set_broadcast(true);
    }

    return send_message(client, msg, secret_code);
}


/** \brief Send a UDP message to the specified \p client.
 *
 * This function sends a UDP message to the specified client. In most
 * cases, you want to send a message using the other two send_message()
 * functions. If you have your own instance of a udp_client object,
 * then you are free to use this function instead.
 *
 * \todo
 * I think it would be possible to have this function as part of the
 * udp_client class instead. Since it is static, there is no real
 * need for any specific field from the UDP servero.
 *
 * \param[in] client  The client where the message gets sent.
 * \param[in] msg  The message to send to the destination.
 * \param[in] secret_code  The secret code to send along the message.
 *
 * \return true when the message was sent, false otherwise.
 */
bool udp_server_message_connection::send_message(
      udp_client & client
    , message const & msg
    , std::string const & secret_code)
{
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

    // TODO: this maximum size needs to be checked dynamically;
    //       also it's not forbidden to send a multiple packet
    //       UDP buffer, it's just more likely to fail
    //
    if(buf.length() > DATAGRAM_MAX_SIZE)
    {
        // packet too large for our buffers
        //
        throw invalid_message(
                  "message too large ("
                + std::to_string(buf.length())
                + " bytes) for a UDP server (max: "
                  BOOST_PP_STRINGIZE(DATAGRAM_MAX_SIZE)
                  ")");
    }

    if(client.send(buf.data(), buf.length()) != static_cast<ssize_t>(buf.length())) // we do not send the '\0'
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << SNAP_LOG_FIELD("errno", std::to_string(e))
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
                        << "no secret_code=... parameter was expected (missing set_secret_code() call for this application?)"
                        << SNAP_LOG_SEND;
                }
            }
            else if(!expected.empty())
            {
                // secret code is missing from incoming message
                //
                SNAP_LOG_ERROR
                    << "the incoming message was expected to have a secret_code parameter, message ignored."
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


/** \brief Set the secret code to be used along messages.
 *
 * This allows the users to call the send_message() function that does
 * not include a \p secret_code parameter and still make the function
 * work as expected.
 *
 * This should be set at initialization time.
 *
 * \param[in] secret_code  The secret code to use along the UDP messages.
 */
void udp_server_message_connection::set_secret_code(std::string const & secret_code)
{
    f_secret_code = secret_code;
}


/** \brief Retrieve the secret code.
 *
 * This function is the converse of the set_secret_code(). It retrieves the
 * secret code.
 *
 * Note that the functions called with a secret code do not save that secret
 * code in the object. You have to explicitly call the set_secret_code()
 * function to do so.
 *
 * \note
 * The send_message() accepting an ed::message and a bool (cache), makes
 * use of this function to retrieve the secret code and call the
 * send_message() accepting an ed::message and a string.
 *
 * \return The secret code as set with the set_secret_code().
 */
std::string udp_server_message_connection::get_secret_code() const
{
    return f_secret_code;
}



} // namespace ed
// vim: ts=4 sw=4 et
