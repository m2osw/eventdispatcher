// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */


// self
//
#include    "eventdispatcher/udp_client.h"


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initialize a UDP client object.
 *
 * This function initializes the UDP client object using the address and the
 * port as specified.
 *
 * The port is expected to be a host side port number (i.e. 59200).
 *
 * The \p addr parameter is a textual address. It may be an IPv4 or IPv6
 * address and it can represent a host name or an address defined with
 * just numbers. If the address cannot be resolved then an error occurs
 * and the constructor throws.
 *
 * \note
 * The socket is open in this process. If you fork() or exec() then the
 * socket will be closed by the operating system.
 *
 * \warning
 * We only make use of the first address found by getaddrinfo(). All
 * the other addresses are ignored.
 *
 * \exception udp_client_server_runtime_error
 * The server could not be initialized properly. Either the address cannot be
 * resolved, the port is incompatible or not available, or the socket could
 * not be created.
 *
 * \param[in] address  The address and port.
 */
udp_client::udp_client(addr::addr const & address)
    : udp_base(address)
{
}


/** \brief Clean up the UDP client object.
 *
 * This function frees the address information structure and close the socket
 * before returning.
 */
udp_client::~udp_client()
{
}


/** \brief Send a message through this UDP client.
 *
 * This function sends \p msg through the UDP client socket. The function
 * cannot be used to change the destination as it was defined when creating
 * the udp_client object.
 *
 * The size must be small enough for the message to fit. In most cases, we
 * use these in Snap! to send very small signals (i.e. 4 bytes commands.)
 * Any data we would want to share remains in the Cassandra database so
 * that way we can avoid losing it because of a UDP message.
 *
 * \note
 * The send may fail with EAGAIN, EWOULDBLOCK, or ENOBUFS which all mean
 * that the attempt can be tried again. The ENOBUFS means that a new buffer
 * could not be allocated, not that the UDP queue is full or that packets
 * are being lost because too many are being sent in a row. Also, you want
 * to control the size of your buffer with get_mss_size(). UDP buffers that
 * are larger will be broken up in multiple packets and that increases the
 * chance that the packet never arrives. Also over a certain size (probably
 * around 64K in IPv4) the ENOBUFS automatically happens. Note that IPv6
 * allows for much buffer packets. This is not automatically a good idea
 * unless the number of packets is quite small because when it fails, you
 * have to resend a very large packet...
 *
 * \note
 * To avoid drop of UDP messages, you need to time your calls to the send()
 * function taking in account the amount of data being sent and the network
 * speed. On the lo network, the throughput is very high and packets can be
 * really large (nearly 64Kb). On a local network, check the size with the
 * get_mss_size() function and (TBD how?) check the speed. You certainly
 * can use up to about 95% of the speed without much problems assuming that
 * the pipe is only used by these UDP packets. Too much and the dropping is
 * going to increase steadily.
 *
 * \todo
 * See whether we could always use the IPv6 to avoid the duplication of the
 * client.
 *
 * \param[in] msg  The message to send.
 * \param[in] size  The number of bytes representing this message.
 *
 * \return -1 if an error occurs, otherwise the number of bytes sent. errno
 * is set accordingly on error.
 */
int udp_client::send(char const * msg, size_t size)
{
    if(f_address.is_ipv4())
    {
        sockaddr_in a;
        f_address.get_ipv4(a);
        return static_cast<int>(sendto(
                      f_socket.get()
                    , msg
                    , size
                    , 0
                    , reinterpret_cast<sockaddr const *>(&a)
                    , sizeof(a)));
    }
    else
    {
        sockaddr_in6 a;
        f_address.get_ipv6(a);
        return static_cast<int>(sendto(
                      f_socket.get()
                    , msg
                    , size
                    , 0
                    , reinterpret_cast<sockaddr const *>(&a)
                    , sizeof(a)));
    }
}



} // namespace ed
// vim: ts=4 sw=4 et
