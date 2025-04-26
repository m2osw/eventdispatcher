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
#pragma once

/** \file
 * \brief A TCP connection, client side.
 *
 * This class implements a connection using TCP. This is the client side.
 * This connection connects to the server listening on a TCP port.
 */

// self
//
#include    <eventdispatcher/connection.h>
#include    <eventdispatcher/tcp_bio_client.h>



namespace ed
{



class tcp_client_connection
    : public connection
    , public tcp_bio_client
{
public:
    typedef std::shared_ptr<tcp_client_connection>    pointer_t;

                                tcp_client_connection(
                                      addr::addr const & address
                                    , mode_t mode = mode_t::MODE_PLAIN);

    addr::addr const &          get_remote_address() const;

    // connection implementation
    virtual bool                is_reader() const override;
    virtual int                 get_socket() const override;

    // new callbacks
    virtual ssize_t             read(void * buf, size_t count);
    virtual ssize_t             write(void const * buf, size_t count);

private:
    addr::addr const            f_remote_address = addr::addr();
};



} // namespace ed
// vim: ts=4 sw=4 et
