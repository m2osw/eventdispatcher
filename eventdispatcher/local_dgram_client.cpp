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
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */


// self
//
#include    "eventdispatcher/local_dgram_client.h"


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initialize a local datagram client object.
 *
 * This function initializes the local datagram client object using the
 * address and the sequential flag as specified.
 *
 * The \p address parameter is a Unix address. It may be a file, abstract
 * or unnamed.
 *
 * \note
 * The sequential flag means we use a SOCK_SEQPACKET type instead of
 * SOCK_DGRAM. Both essentially work the same way except that the
 * SOCK_SEQPACKET has a promess to keep the packets in sequential
 * order (the order in which they were sent).
 *
 * \note
 * The socket is open in this process. If you fork() or exec() then the
 * socket will be closed by the operating system if the close_on_exec
 * was set to true.
 *
 * \param[in] address  The address to send data to.
 * \param[in] sequential  Whether to force sequential sends.
 * \param[in] close_on_exec  Whether the socket has to be closed on exec.
 */
local_dgram_client::local_dgram_client(
              addr::unix const & address
            , bool sequential
            , bool close_on_exec)
    : local_dgram_base(address, sequential, close_on_exec)
{
}


/** \brief Send a message through this UDP client.
 *
 * This function sends \p msg through the UDP client socket. The function
 * cannot be used to change the destination as it was defined when creating
 * the local_dgram_client object.
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
 * To avoid loss of datagram messages, you need to time your calls to the send()
 * function taking in account the amount of data being sent and the network
 * speed. With a Unix socket, the throughput is very high and packets can be
 * really large (often 128Kb or even 256Kb). You can check the size with the
 * get_mss_size() function and (TBD how?) check the speed. You certainly
 * can use up to about 95% of the speed without much problems assuming that
 * the pipe is only used by these datagram packets. Too much and the
 * dropping is going to increase steadily. In also depends on how fast
 * the receiver retrieves the packets.
 *
 * \param[in] msg  The message to send.
 * \param[in] size  The number of bytes representing this message.
 *
 * \return -1 if an error occurs, otherwise the number of bytes sent. errno
 * is set accordingly on error.
 */
int local_dgram_client::send(char const * msg, size_t size)
{
    struct sockaddr_un un;
    f_address.get_un(un);
    return static_cast<int>(sendto(
              f_socket.get()
            , msg
            , size
            , 0
            , reinterpret_cast<sockaddr const *>(&un)
            , sizeof(un)));
}



} // namespace ed
// vim: ts=4 sw=4 et
