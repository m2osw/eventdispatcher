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
#include "eventdispatcher/udp_client_server.h"


// snapwebsites lib
//
#include "snaplogger/message.h"


// libaddr lib
//
#include <libaddr/iface.h>


// C++ lib
//
#include <sstream>


// C lib
//
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <poll.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>


// last include
//
#include "snapdev/poison.h"




namespace udp_client_server
{

// ========================= BASE =========================

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
        throw udp_client_server_parameter_error("the address cannot be an empty string");
    }

    // the port must be between 0 and 65535
    // (although 0 won't work right as far as I know)
    //
    if(f_port < 0 || f_port >= 65536)
    {
        throw udp_client_server_parameter_error("invalid port for a client socket");
    }

    // for the getaddrinfo() function, convert the port to a string
    //
    std::stringstream decimal_port;
    decimal_port << f_port;
    std::string port_str(decimal_port.str());

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
        throw udp_client_server_runtime_error(("invalid address or port: \"" + addr + ":" + port_str + "\"").c_str());
    }
    f_addrinfo = raii_addrinfo_t(info);

    // now create the socket with the very first socket family
    //
    f_socket.reset(socket(f_addrinfo->ai_family, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP));
    if(f_socket == nullptr)
    {
        throw udp_client_server_runtime_error(("could not create socket for: \"" + addr + ":" + port_str + "\"").c_str());
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
                struct ifreq ifr;
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








// ========================= CLIENT =========================

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
 * and constructor throws.
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
 * \param[in] addr  The address to convert to a numeric IP.
 * \param[in] port  The port number.
 * \param[in] family  The family used to search for 'addr'.
 */
udp_client::udp_client(std::string const & addr, int port, int family)
    : udp_base(addr, port, family)
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
 * The size must be small enough for the message to fit. In most cases we
 * use these in Snap! to send very small signals (i.e. 4 bytes commands.)
 * Any data we would want to share remains in the Cassandra database so
 * that way we can avoid losing it because of a UDP message.
 *
 * \param[in] msg  The message to send.
 * \param[in] size  The number of bytes representing this message.
 *
 * \return -1 if an error occurs, otherwise the number of bytes sent. errno
 * is set accordingly on error.
 */
int udp_client::send(char const * msg, size_t size)
{
    return static_cast<int>(sendto(f_socket.get(), msg, size, 0, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen));
}






// ========================= SEVER =========================

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

        SNAP_LOG_ERROR("the bind() function failed with errno: ")
                (e)
                (" (")
                (strerror(e))
                ("); address length ")
                (f_addrinfo->ai_addrlen)
                (" and address is \"")
                (addr_buf)
                ("\"");
        throw udp_client_server_runtime_error("could not bind UDP socket with: \"" + f_addr + ":" + std::to_string(port) + "\"");
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
        struct addrinfo * a(nullptr);
        r = getaddrinfo(multicast_addr->c_str(), port_str.c_str(), &hints, &a);
        if(r != 0 || a == nullptr)
        {
            throw udp_client_server_runtime_error("invalid address or port for UDP socket: \"" + addr + ":" + port_str + "\"");
        }

        // both addresses must have the right size
        //
        if(a->ai_addrlen != sizeof(mreq.imr_multiaddr)
        || f_addrinfo->ai_addrlen != sizeof(mreq.imr_address))
        {
            throw udp_client_server_runtime_error("invalid address type for UDP multicast: \"" + addr + ":" + port_str
                                                        + "\" or \"" + *multicast_addr + ":" + port_str + "\"");
        }

        memcpy(&mreq.imr_multiaddr, a->ai_addr->sa_data, sizeof(mreq.imr_multiaddr));
        memcpy(&mreq.imr_address, f_addrinfo->ai_addr->sa_data, sizeof(mreq.imr_address));
        mreq.imr_ifindex = 0;   // no specific interface

        r = setsockopt(f_socket.get(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        if(r < 0)
        {
            int const e(errno);
            throw udp_client_server_runtime_error("IP_ADD_MEMBERSHIP failed for: \"" + addr + ":" + port_str
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
            SNAP_LOG_WARNING("could not set IP_MULTICAST_ALL to zero, e = ")
                            (e)
                            (" -- ")
                            (strerror(e));
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
    struct pollfd fd;
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




} // namespace udp_client_server

// vim: ts=4 sw=4 et
