// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */


// self
//
#include    "eventdispatcher/udp_base.h"

#include    "eventdispatcher/exception.h"


// libaddr
//
#include    <libaddr/iface.h>


// C++
//
#include    <sstream>


// C
//
#include    <net/if.h>
#include    <netinet/ip.h>
#include    <netinet/udp.h>
#include    <sys/ioctl.h>


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
 * The \p address parameter is a libaddr address. It may be an IPv4 or IPv6
 * address and it correspond to a host name or be an address defined with
 * just numbers.
 *
 * \note
 * The socket is open in this process. If you fork() and exec() then the
 * socket gets closed by the operating system (i.e. close on exec()).
 *
 * \exception udp_client_server_parameter_error
 * The \p address parameter is empty or the port is out of the supported range.
 *
 * \exception udp_client_server_runtime_error
 * The server could not be initialized properly. Either the address cannot be
 * resolved, the port is incompatible or not available, or the socket could
 * not be created.
 *
 * \param[in] address  The address and port.
 */
udp_base::udp_base(addr::addr const & address)
    : f_address(address)
{
    // create the socket
    //
    f_socket.reset(socket(f_address.get_family(), SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP));
    if(f_socket == nullptr)
    {
        std::stringstream ss;
        ss << "could not create socket for: \""
           << f_address.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT)
           << "\".";
        throw runtime_error(ss.str());
    }
}


/** \brief Clean up the UDP base class.
 *
 * This function is here because it handles the creation of the virtual table.
 */
udp_base::~udp_base()
{
}


/** \brief Retrieve a copy of the socket identifier.
 *
 * This function return the socket identifier as returned by the socket()
 * function. This can be used to change some flags.
 *
 * \return The socket used by this UDP client.
 */
int udp_base::get_socket() const
{
    return f_socket.get();
}


/** \brief Set whether this UDP socket is to be used to broadcast messages.
 *
 * This function sets the BROADCAST flag on the socket. This is important
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
 * flag on this UDP socket.
 */
void udp_base::set_broadcast(bool state)
{
    int const value(state ? 1 : 0);
    setsockopt(f_socket.get(), SOL_SOCKET, SO_BROADCAST, &value, sizeof(value));
}


/** \brief Retrieve the size of the MTU on that connection.
 *
 * Linux offers a ioctl() function to retrieve the MTU's size. This
 * function uses that and returns the result. If the call fails,
 * then the function returns -1.
 *
 * The function returns the MTU's size of the socket on this side.
 * If you want to communicate effectively with another system, you
 * want to also ask about the MTU on the other side of the socket.
 *
 * \note
 * MTU stands for Maximum Transmission Unit.
 *
 * \note
 * PMTUD stands for Path Maximum Transmission Unit Discovery.
 *
 * \note
 * PLPMTU stands for Packetization Layer Path Maximum Transmission Unit
 * Discovery.
 *
 * \todo
 * We need to support the possibly of dynamically changing MTU size
 * that the Internet may generate (or even a LAN if you let people
 * tweak their MTU "randomly".) This is done by preventing
 * defragmentation (see IP_NODEFRAG in `man 7 ip`) and also by
 * asking for MTU size discovery (IP_MTU_DISCOVER). The size
 * discovery changes over time as devices on the MTU path (the
 * route taken by the packets) changes over time. The idea is
 * to find the smallest MTU size of the MTU path and use that
 * to send packets of that size at the most. Note that packets
 * are otherwise automatically broken in smaller chunks and
 * rebuilt on the other side, but that is not efficient if you
 * expect to lose quite a few packets along the way. The limit
 * for chunked packets is a little under 64Kb.
 *
 * \note
 * errno is either EBADF or set by ioctl().
 *
 * \sa
 * See `man 7 netdevice`
 *
 * \return -1 if the MTU could not be retrieved, the MTU's size otherwise.
 */
int udp_base::get_mtu_size() const
{
    if(f_socket != nullptr
    && f_mtu_size == 0)
    {
        std::string iface_name;
        addr::iface::pointer_t i(find_addr_interface(f_address));
        if(i != nullptr)
        {
            iface_name = i->get_name();
        }

        if(iface_name.empty())
        {
            f_mtu_size = -1;
            errno = EBADF;
        }
        else
        {
            ifreq ifr = {};
            strncpy(ifr.ifr_name, iface_name.c_str(), sizeof(ifr.ifr_name) - 1);
            if(ioctl(f_socket.get(), SIOCGIFMTU, &ifr) == 0)
            {
                f_mtu_size = ifr.ifr_mtu;
            }
            else
            {
                f_mtu_size = -1;
                // errno -- defined by ioctl()
            }
        }
    }

    return f_mtu_size;
}


/** \brief Determine the size of the data buffer we can use.
 *
 * This function gets the MTU of the connection (i.e. not the PMTUD
 * or PLPMTUD yet...) and subtract the space necessary for the IP and
 * UDP headers. This is called the Maximum Segment Size (MSS).
 *
 * \todo
 * If the IP address (in f_addr) is an IPv6, then we need to switch to
 * the corresponding IPv6 subtractions.
 *
 * \todo
 * Look into the the IP options because some options add to the size
 * of the IP header. It's incredible that we have to take care of that
 * on our end!
 *
 * \todo
 * For congestion control, read more as described on ietf.org:
 * https://tools.ietf.org/html/rfc8085
 *
 * \todo
 * The sizes that will always work (as long as all the components of the
 * path are working as per the UDP RFC) are (1) for IPv4, 576 bytes, and
 * (2) for IPv6, 1280 bytes. This size is called EMTU_S which stands for
 * "Effective Maximum Transmission Unit for Sending."
 *
 * \return The size of the MMU, which is the MTU minus IP and UDP headers.
 */
int udp_base::get_mss_size() const
{
    // where these structures are defined
    //
    // ether_header -- /usr/include/net/ethernet.h
    // iphdr -- /usr/include/netinet/ip.h
    // udphdr -- /usr/include/netinet/udp.h
    //
    int const mtu(get_mtu_size()
            //- sizeof(ether_header)    // WARNING: this is for IPv4 only -- this is "transparent" to the MTU (i.e. it wraps the 1,500 bytes)
            //- ETHER_CRC_LEN           // this is the CRC for the ethernet which appears at the end of the packet
            - sizeof(iphdr)             // WARNING: this is for IPv4 only
            //- ...                     // the IP protocol accepts options!
            - sizeof(udphdr)
        );

    return mtu <= 0 ? -1 : mtu;
}


/** \brief Retrieve a copy of the address.
 *
 * This function returns a copy of the address as it was specified in the
 * constructor. This does not return a canonicalized version of the address.
 *
 * The address cannot be modified. If you need to send data on a different
 * address, create a new UDP client.
 *
 * \note
 * If you set the port to 0 and then do a bind (i.e. create a server,
 * listening socket), then the port will automatically be assigned
 * by the network stack. This is allowed for the UDP server.
 *
 * \return A string with a copy of the constructor input address.
 */
addr::addr udp_base::get_address() const
{
    return f_address;
}



} // namespace ed
// vim: ts=4 sw=4 et
