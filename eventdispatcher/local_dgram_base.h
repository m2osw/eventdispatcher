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
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */


// snapdev lib
//
#include    <snapdev/raii_generic_deleter.h>


// libaddr lib
//
#include    <libaddr/unix.h>



namespace ed
{




class local_dgram_base
{
public:
    virtual             ~local_dgram_base();

    int                 get_socket() const;
    void                set_broadcast(bool state);

    int                 get_mtu_size() const;
    int                 get_mss_size() const;
    addr::unix          get_address() const;

protected:
                        local_dgram_base(
                                  addr::unix const & address
                                , bool sequential
                                , bool close_on_exec);

    // TODO: convert the port + addr into a libaddr addr object?
    //       (we use the f_addrinfo as is in the sendto() and bind() calls
    //       and use libaddr for the conversions already)
    //
    addr::unix          f_address = addr::unix();
    snapdev::raii_fd_t  f_socket = snapdev::raii_fd_t();
    int                 f_mtu_size = -1;
};



} // namespace ed
// vim: ts=4 sw=4 et
