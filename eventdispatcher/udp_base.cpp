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
#include "eventdispatcher/udp_base.h"

#include "eventdispatcher/exception.h"


// libaddr lib
//
#include <libaddr/iface.h>


// C lib
//
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <sys/ioctl.h>


// last include
//
#include "snapdev/poison.h"




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
 * \exception udp_client_server_parameter_error
 * The \p addr parameter is empty or the port is out of the supported range.
 *
 * \exception udp_client_server_runtime_error
 * The server could not be initialized properly. Either the address cannot be
 * resolved, the port is incompatible or not available, or the socket could
 * not be created.
 *
 * \param[in] addr  The address to convert to a numeric IP.
 * \param[in] port  The port number.
 * \param[in] family  The family used to search for 'addr'.
 */
udp_base::udp_base(std::string const & addr, int port, int family)
    : f_port(port)
    , f_addr(addr)
{
    // the address can't be an empty string
    //
    if(f_addr.empty())
    {
        throw event_dispatcher_invalid_parameter("the address cannot be an empty string");
    }

    // the port must be between 0 and 65535
    // (although 0 won't work right as far as I know)
    //
    if(f_port < 0 || f_port >= 65536)
    {
        throw event_dispatcher_invalid_parameter("invalid port for a client socket");
    }

    // for the getaddrinfo() function, convert the port to a string
    //
    std::string const port_str(std::to_string(f_port));

    // define the getaddrinfo() hints
    // we are only interested by addresses representing datagrams and
    // acceptable by the UDP protocol
    //
    struct addrinfo hints = addrinfo();
    hints.ai_family = family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    // retrieve the list of addresses defined by getaddrinfo()
    //
    struct addrinfo * info;
    int const r(getaddrinfo(addr.c_str(), port_str.c_str(), &hints, &info));
    if(r != 0 || info == nullptr)
    {
        throw event_dispatcher_invalid_parameter("invalid address or port: \"" + addr + ":" + port_str + "\"");
    }
    f_addrinfo = raii_addrinfo_t(info);

    // now create the socket with the very first socket family
    //
    f_socket.reset(socket(f_addrinfo->ai_family, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP));
    if(f_socket == nullptr)
    {
        throw event_dispatcher_runtime_error(("could not create socket for: \"" + addr + ":" + port_str + "\"").c_str());
    }
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
 * We need to support the possibly dynamically changing MTU size
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
 * expect to lose quite a few packets. The limit for chunked
 * packets is a little under 64Kb.
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
        addr::addr a;
        switch(f_addrinfo->ai_family)
        {
        case AF_INET:
            a.set_ipv4(*reinterpret_cast<struct sockaddr_in *>(f_addrinfo->ai_addr));
            break;

        case AF_INET6:
            a.set_ipv6(*reinterpret_cast<struct sockaddr_in6 *>(f_addrinfo->ai_addr));
            break;

        default:
            f_mtu_size = -1;
            errno = EBADF;
            break;

        }
        if(f_mtu_size == 0)
        {
            std::string iface_name;
            addr::iface::pointer_t i(find_addr_interface(a));
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
                ifreq ifr;
                memset(&ifr, 0, sizeof(ifr));
                strncpy(ifr.ifr_name, iface_name.c_str(), sizeof(ifr.ifr_name));
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
 * For congetion control, read more as described on ietf.org:
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


/** \brief Retrieve the port used by this UDP client.
 *
 * This function returns the port used by this UDP client. The port is
 * defined as an integer, host side.
 *
 * \return The port as expected in a host integer.
 */
int udp_base::get_port() const
{
    return f_port;
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
std::string udp_base::get_addr() const
{
    return f_addr;
}



} // namespace ed
// vim: ts=4 sw=4 et
