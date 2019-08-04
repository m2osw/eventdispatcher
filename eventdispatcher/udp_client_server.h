// Event Dispatcher
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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
#pragma once


//#include "snapdispatcher/exception.h"


// snapdev lib
//
#include <snapdev/raii_generic_deleter.h>




// C lib
//
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>



namespace udp_client_server
{



class udp_base
{
public:
    typedef std::unique_ptr<struct addrinfo, snap::raii_pointer_deleter<struct addrinfo, decltype(&::freeaddrinfo), &::freeaddrinfo>> raii_addrinfo_t;

    int                 get_socket() const;
    int                 get_mtu_size() const;
    int                 get_mss_size() const;
    int                 get_port() const;
    std::string         get_addr() const;

protected:
                        udp_base(std::string const & addr, int port, int family);

    // TODO: convert the port + addr into a libaddr addr object?
    //       (we use the f_addrinfo as is in the sendto() and bind() calls, though)
    //
    snap::raii_fd_t     f_socket = snap::raii_fd_t();
    int                 f_port = -1;
    mutable int         f_mtu_size = 0;
    std::string         f_addr = std::string();
    raii_addrinfo_t     f_addrinfo = raii_addrinfo_t();
};


class udp_client
    : public udp_base
{
public:
    typedef std::shared_ptr<udp_client>     pointer_t;

                        udp_client(std::string const & addr, int port, int family = AF_UNSPEC);
                        ~udp_client();

    int                 send(char const * msg, size_t size);

private:
};


class udp_server
    : public udp_base
{
public:
    typedef std::shared_ptr<udp_server>     pointer_t;

                        udp_server(std::string const & addr, int port, int family = AF_UNSPEC, std::string const * multicast_addr = nullptr);
                        ~udp_server();

    int                 recv(char * msg, size_t max_size);
    int                 timed_recv( char * msg, size_t const max_size, int const max_wait_ms );
    std::string         timed_recv( int const bufsize, int const max_wait_ms );

private:
};

} // namespace udp_client_server
// vim: ts=4 sw=4 et
