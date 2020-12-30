// Copyright (c) 2012-2020  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */


// self
//
#include    "eventdispatcher/utils.h"



namespace ed
{




class udp_base
{
public:
    virtual             ~udp_base();

    int                 get_socket() const;
    int                 get_mtu_size() const;
    int                 get_mss_size() const;
    int                 get_port() const;
    std::string         get_addr() const;

protected:
                        udp_base(std::string const & addr, int port, int family);

    // TODO: convert the port + addr into a libaddr addr object?
    //       (we use the f_addrinfo as is in the sendto() and bind() calls
    //       and use libaddr for the convertions already)
    //
    snap::raii_fd_t     f_socket = snap::raii_fd_t();
    int                 f_port = -1;
    mutable int         f_mtu_size = 0;
    std::string         f_addr = std::string();
    raii_addrinfo_t     f_addrinfo = raii_addrinfo_t();
};



} // namespace ed
// vim: ts=4 sw=4 et
