// Event Dispatcher
// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "eventdispatcher/udp_server.h"

#include "eventdispatcher/exception.h"


// snapwebsites lib
//
#include "snaplogger/message.h"


// C lib
//
#include <arpa/inet.h>
#include <poll.h>
#include <string.h>


// last include
//
#include "snapdev/poison.h"




namespace ed
{



/** \brief Initialize a UDP server object.
 *
 * This function initializes a UDP server object making it ready to
 * receive messages.
 *
 * The server address and port are specified in the constructor so
 * if you need to receive messages from several different addresses
 * and/or port, you'll have to create a server for each.
 *
 * The address is a string and it can represent an IPv4 or IPv6
 * address.
 *
 * Note that this function calls bind() to listen to the socket
 * at the specified address. To accept data on different UDP addresses
 * and ports, multiple UDP servers must be created.
 *
 * \note
 * The socket is open in this process. If you fork() or exec() then the
 * socket will be closed by the operating system.
 *
 * \warning
 * We only make use of the first address found by getaddrinfo(). All
 * the other addresses are ignored.
 *
 * \warning
 * Remember that the multicast feature under Linux is shared by all
 * processes running on that server. Any one process can listen for
 * any and all multicast messages from any other process. Our
 * implementation limits the multicast from a specific IP. However.
 * other processes can also receive you packets and there is nothing
 * you can do to prevent that.
 *
 * \exception udp_client_server_runtime_error
 * The udp_client_server_runtime_error exception is raised when the address
 * and port combinaison cannot be resolved or if the socket cannot be
 * opened.
 *
 * \param[in] addr  The address we receive on.
 * \param[in] port  The port we receive from.
 * \param[in] family  The family used to search for 'addr'.
 * \param[in] multicast_addr  A multicast address.
 */
udp_server::udp_server(std::string const & addr, int port, int family, std::string const * multicast_addr)
    : udp_base(addr, port, family)
{
    // bind to the very first address
    //
    int r(bind(f_socket.get(), f_addrinfo->ai_addr, f_addrinfo->ai_addrlen));
    if(r != 0)
    {
        int const e(errno);

        // reverse the address from the f_addrinfo so we know exactly
        // which one was picked
        //
        char addr_buf[256];
        switch(f_addrinfo->ai_family)
        {
        case AF_INET:
            inet_ntop(AF_INET
                    , &reinterpret_cast<struct sockaddr_in *>(f_addrinfo->ai_addr)->sin_addr
                    , addr_buf
                    , sizeof(addr_buf));
            break;

        case AF_INET6:
            inet_ntop(AF_INET6
                    , &reinterpret_cast<struct sockaddr_in6 *>(f_addrinfo->ai_addr)->sin6_addr
                    , addr_buf
                    , sizeof(addr_buf));
            break;

        default:
            strncpy(addr_buf, "Unknown Adress Family", sizeof(addr_buf));
            break;

        }

        SNAP_LOG_ERROR
                << "the bind() function failed with errno: "
                << e
                << " ("
                << strerror(e)
                << "); address length "
                << f_addrinfo->ai_addrlen
                << " and address is \""
                << addr_buf
                << "\"";
        throw event_dispatcher_runtime_error("could not bind UDP socket to \"" + f_addr + ":" + std::to_string(port) + "\"");
    }

    // are we creating a server to listen to multicast packets?
    //
    if(multicast_addr != nullptr)
    {
        struct ip_mreqn mreq;

        std::stringstream decimal_port;
        decimal_port << f_port;
        std::string port_str(decimal_port.str());

        struct addrinfo hints = addrinfo();
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        // we use the multicast address, but the same port as for
        // the other address
        //
        addrinfo * a(nullptr);
        r = getaddrinfo(multicast_addr->c_str(), port_str.c_str(), &hints, &a);
        if(r != 0 || a == nullptr)
        {
            throw event_dispatcher_runtime_error("invalid address or port for UDP socket: \"" + addr + ":" + port_str + "\"");
        }

        // both addresses must have the right size
        //
        if(a->ai_addrlen != sizeof(mreq.imr_multiaddr)
        || f_addrinfo->ai_addrlen != sizeof(mreq.imr_address))
        {
            throw event_dispatcher_runtime_error("invalid address type for UDP multicast: \"" + addr + ":" + port_str
                                                        + "\" or \"" + *multicast_addr + ":" + port_str + "\"");
        }

        memcpy(&mreq.imr_multiaddr, a->ai_addr->sa_data, sizeof(mreq.imr_multiaddr));
        memcpy(&mreq.imr_address, f_addrinfo->ai_addr->sa_data, sizeof(mreq.imr_address));
        mreq.imr_ifindex = 0;   // no specific interface

        r = setsockopt(f_socket.get(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        if(r < 0)
        {
            int const e(errno);
            throw event_dispatcher_runtime_error("IP_ADD_MEMBERSHIP failed for: \"" + addr + ":" + port_str
                                                        + "\" or \"" + *multicast_addr + ":" + port_str + "\", "
                                                        + std::to_string(e) + strerror(e));
        }

        // setup the multicast to 0 so we don't receive other's
        // messages; apparently the default would be 1
        //
        int multicast_all(0);
        r = setsockopt(f_socket.get(), IPPROTO_IP, IP_MULTICAST_ALL, &multicast_all, sizeof(multicast_all));
        if(r < 0)
        {
            // things should still work if the IP_MULTICAST_ALL is not
            // set as we want it
            //
            int const e(errno);
            SNAP_LOG_WARNING
                    << "could not set IP_MULTICAST_ALL to zero, e = "
                    << e
                    << " -- "
                    << strerror(e);
        }
    }
}


/** \brief Clean up the UDP server.
 *
 * This function frees the address info structures and close the socket.
 */
udp_server::~udp_server()
{
}


/** \brief Wait on a message.
 *
 * This function waits until a message is received on this UDP server.
 * There are no means to return from this function except by receiving
 * a message. Remember that UDP does not have a connect state so whether
 * another process quits does not change the status of this UDP server
 * and thus it continues to wait forever.
 *
 * Note that you may change the type of socket by making it non-blocking
 * (use the get_socket() to retrieve the socket identifier) in which
 * case this function will not block if no message is available. Instead
 * it returns immediately.
 *
 * \param[in] msg  The buffer where the message is saved.
 * \param[in] max_size  The maximum size the message (i.e. size of the \p msg buffer.)
 *
 * \return The number of bytes read or -1 if an error occurs.
 */
int udp_server::recv(char * msg, size_t max_size)
{
    return static_cast<int>(::recv(f_socket.get(), msg, max_size, 0));
}


/** \brief Wait for data to come in.
 *
 * This function waits for a given amount of time for data to come in. If
 * no data comes in after max_wait_ms, the function returns with -1 and
 * errno set to EAGAIN.
 *
 * The socket is expected to be a blocking socket (the default,) although
 * it is possible to setup the socket as non-blocking if necessary for
 * some other reason.
 *
 * This function blocks for a maximum amount of time as defined by
 * max_wait_ms. It may return sooner with an error or a message.
 *
 * \param[in] msg  The buffer where the message will be saved.
 * \param[in] max_size  The size of the \p msg buffer in bytes.
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for a message.
 *
 * \return -1 if an error occurs or the function timed out, the number of bytes received otherwise.
 */
int udp_server::timed_recv(char * msg, size_t const max_size, int const max_wait_ms)
{
    pollfd fd;
    fd.events = POLLIN | POLLPRI | POLLRDHUP;
    fd.fd = f_socket.get();
    int const retval(poll(&fd, 1, max_wait_ms));

//    fd_set s;
//    FD_ZERO(&s);
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wold-style-cast"
//    FD_SET(f_socket.get(), &s);
//#pragma GCC diagnostic pop
//    struct timeval timeout;
//    timeout.tv_sec = max_wait_ms / 1000;
//    timeout.tv_usec = (max_wait_ms % 1000) * 1000;
//    int const retval(select(f_socket.get() + 1, &s, nullptr, &s, &timeout));
    if(retval == -1)
    {
        // poll() sets errno accordingly
        return -1;
    }
    if(retval > 0)
    {
        // our socket has data
        return static_cast<int>(::recv(f_socket.get(), msg, max_size, 0));
    }

    // our socket has no data
    errno = EAGAIN;
    return -1;
}


/** \brief Wait for data to come in, but return a std::string.
 *
 * This function waits for a given amount of time for data to come in. If
 * no data comes in after max_wait_ms, the function returns with -1 and
 * errno set to EAGAIN.
 *
 * The socket is expected to be a blocking socket (the default,) although
 * it is possible to setup the socket as non-blocking if necessary for
 * some other reason.
 *
 * This function blocks for a maximum amount of time as defined by
 * max_wait_ms. It may return sooner with an error or a message.
 *
 * \param[in] bufsize  The maximum size of the returned string in bytes.
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for a message.
 *
 * \return received string. nullptr if error.
 *
 * \sa timed_recv()
 */
std::string udp_server::timed_recv( int const bufsize, int const max_wait_ms )
{
    std::vector<char> buf;
    buf.resize( bufsize + 1, '\0' ); // +1 for ending \0
    int const r(timed_recv( &buf[0], bufsize, max_wait_ms ));
    if( r <= -1 )
    {
        // Timed out, so return empty string.
        // TBD: could std::string() smash errno?
        //
        return std::string();
    }

    // Resize the buffer, then convert to std string
    //
    buf.resize( r + 1, '\0' );

    std::string word;
    word.resize( r );
    std::copy( buf.begin(), buf.end(), word.begin() );

    return word;
}




} // namespace ed
// vim: ts=4 sw=4 et
