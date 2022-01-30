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
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */


// self
//
#include    "eventdispatcher/local_dgram_base.h"

#include    "eventdispatcher/exception.h"


// libaddr lib
//
//#include    <libaddr/iface.h>


// C lib
//
//#include    <net/if.h>
//#include    <netinet/ip.h>
//#include    <netinet/udp.h>
//#include    <sys/ioctl.h>


// last include
//
#include    <snapdev/poison.h>




namespace ed
{



/** \brief Initialize a UDP base object.
 *
 * This function initializes the UDP base object using the address and the
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
 * The socket is open in this process. If you fork() and exec() then the
 * socket gets closed by the operating system (i.e. close on exec()).
 *
 * \warning
 * We only make use of the first address found by getaddrinfo(). All
 * the other addresses are ignored.
 *
 * \todo
 * Add a constructor that supports a libaddr::addr object instead of
 * just a string address.
 *
 * \exception event_dispatcher_invalid_parameter
 * The \p addr parameter is empty or the port is out of the supported range.
 *
 * \exception event_dispatcher_runtime_error
 * The server could not be initialized properly. Either the address cannot be
 * resolved, the port is incompatible or not available, or the socket could
 * not be created.
 *
 * \param[in] address  The address to connect/listen to.
 * \param[in] sequential  Whether the packets have to be 100% sequential.
 * \param[in] close_on_exec  Whether the socket has to be closed on execve().
 */
local_dgram_base::local_dgram_base(
              addr::unix const & address
            , bool sequential
            , bool close_on_exec)
    : f_address(address)
{
    int type(sequential ? SOCK_SEQPACKET : SOCK_DGRAM);
    if(close_on_exec)
    {
        type |= SOCK_CLOEXEC;
    }

    f_socket.reset(socket(AF_UNIX, type, 0));
    if(f_socket == nullptr)
    {
        throw event_dispatcher_runtime_error(
              "could not create socket for: \""
            + f_address.to_uri()
            + "\".");
    }

    // get the "MTU" maximum size right away for
    //   (1) it is really fast; and
    //   (2) it is going to work right before a first write() but may not
    //       be if a write() was not yet fully processed
    //
    socklen_t optlen;
    optlen = sizeof(f_mtu_size);
    int const r(getsockopt(
              f_socket.get()
            , SOL_SOCKET
            , SO_SNDBUF
            , &f_mtu_size
            , &optlen));
    if(r != 0)
    {
        throw event_dispatcher_runtime_error(
              "could not retrieve \"MTU\" size for: \""
            + f_address.to_uri()
            + "\".");
    }
}


/** \brief The local datagram destructor.
 *
 * To avoid potential errors with virtual destruction, we have a virtual
 * destructor in this base class.
 */
local_dgram_base::~local_dgram_base()
{
}


/** \brief Retrieve a copy of the socket identifier.
 *
 * This function return the socket identifier as returned by the socket()
 * function. This can be used to change some flags.
 *
 * \return The socket used by this UDP client.
 */
int local_dgram_base::get_socket() const
{
    return f_socket.get();
}


/** \brief Set whether this UDP socket is to be used to broadcast messages.
 *
 * This function sets the BROADCAST flagon the socket. This is important
 * because by default it is expected that the socket is not used in
 * broadcast mode. This makes sure that was your intention.
 *
 * \note
 * We do not try to automatically set the flag for (1) the OS implementation
 * expects the end user application to systematically set the flag if
 * required and (2) it's complicated to know whether the address represents
 * the broadcast address (i.e. you need to get the info on the corresponding
 * interface to get the network mask, see whether the interface supports
 * broadcasting, etc.) We'll eventually implement that test in our
 * libaddr library one day. However, that would be a test we use in the
 * send() function to catch errors early (i.e. determine whether the
 * socket can be sent to in the current state).
 *
 * \param[in] state  Whether to set (true) or remove (false) the broadcast
 * flag on this Unix datagram socket.
 */
void local_dgram_base::set_broadcast(bool state)
{
    int const value(state ? 1 : 0);
    setsockopt(f_socket.get(), SOL_SOCKET, SO_BROADCAST, &value, sizeof(value));
}


/** \brief Retrieve the size of the MTU on that connection.
 *
 * The "MTU" of the AF_UNIX message is defined by the largest allocatable
 * page of memory. This is defined in this file:
 *
 * /proc/sys/net/core/wmem_max
 *
 * Note that to get the maximum size of your message, you want to use
 * the get_mss_size() instead. The MTU size is the entire packet including
 * headers.
 *
 * \return -1 if the MTU could not be retrieved, the MTU's size otherwise.
 */
int local_dgram_base::get_mtu_size() const
{
    return f_mtu_size;
}


/** \brief Determine the size of the data buffer we can use.
 *
 * This function gets the MTU and then subtract the possible header data
 * of the packet to 
 *
 * \return The size of the MMU, which is the MTU minus the headers.
 */
int local_dgram_base::get_mss_size() const
{
    int const mss(get_mtu_size());
    return mss < 32 ? -1 : mss - 32;      // it looks like the header uses 32 bytes
}


/** \brief Retrieve a copy of the address.
 *
 * This function returns a copy of the address as it was specified in the
 * constructor. This does not return a canonicalized version of the address.
 *
 * The address cannot be modified. If you need to send data on a different
 * address, create a new UDP client.
 *
 * \return A string with a copy of the constructor input address.
 */
addr::unix local_dgram_base::get_address() const
{
    return f_address;
}



} // namespace ed
// vim: ts=4 sw=4 et
