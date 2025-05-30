// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Declaration of the AF_UNIX client connection.
 *
 * This is the base connection class for the Unix socket handling. You
 * probably want to use the local_stream_client_permanent_message_connection
 * instead.
 */

// self
// 
#include    <eventdispatcher/connection.h>


// libaddr
// 
#include    <libaddr/addr_unix.h>


// snapdev
//
#include    <snapdev/raii_generic_deleter.h>




namespace ed
{




class local_stream_client_connection
    : public connection
{
public:
    typedef std::shared_ptr<local_stream_client_connection>     pointer_t;

                        local_stream_client_connection(
                                  addr::addr_unix const & address
                                , bool blocking = false
                                , bool close_on_exec = true);
    virtual             ~local_stream_client_connection() override;

    void                close();
    addr::addr_unix     get_address() const;

    // connection implementation
    //
    virtual bool        is_reader() const override;
    virtual int         get_socket() const override;

    // new callbacks
    //
    virtual ssize_t     read(char * buf, size_t size);
    virtual ssize_t     write(void const * buf, size_t size);

private:
    addr::addr_unix     f_address = addr::addr_unix();
    snapdev::raii_fd_t  f_socket = snapdev::raii_fd_t();
};



} // namespace ed
// vim: ts=4 sw=4 et
