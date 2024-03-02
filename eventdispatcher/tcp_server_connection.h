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
 * \brief TCP Server connection class.
 *
 * Class used to handle a TCP connection.
 */

// self
//
#include    <eventdispatcher/connection.h>
#include    <eventdispatcher/tcp_bio_server.h>



namespace ed
{



class tcp_server_connection
    : public connection
    , public tcp_bio_server
{
public:
    typedef std::shared_ptr<tcp_server_connection>    pointer_t;

                                tcp_server_connection(
                                      addr::addr const & addr
                                    , std::string const & certificate
                                    , std::string const & private_key
                                    , mode_t mode = mode_t::MODE_PLAIN
                                    , int max_connections = -1
                                    , bool reuse_addr = false);

    // connection implementation
    //
    virtual bool                is_listener() const override;
    virtual int                 get_socket() const override;
};



} // namespace ed
// vim: ts=4 sw=4 et
