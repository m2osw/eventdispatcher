// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher/connection.h"
#include    "eventdispatcher/tcp_bio_client.h"


// C lib
//
#include    <sys/socket.h>



namespace ed
{



class tcp_server_client_connection
    : public connection
    //, public tcp_client -- this will not work without some serious re-engineering of the tcp_client class
{
public:
    typedef std::shared_ptr<tcp_server_client_connection>    pointer_t;

                                tcp_server_client_connection(tcp_bio_client::pointer_t client);
    virtual                     ~tcp_server_client_connection() override;

    void                        close();
    size_t                      get_client_address(sockaddr_storage & address) const;
    std::string                 get_client_addr() const;
    int                         get_client_port() const;
    std::string                 get_client_addr_port() const;

    // connection implementation
    virtual bool                is_reader() const override;
    virtual int                 get_socket() const override;

    // new callbacks
    virtual ssize_t             read(void * buf, size_t count);
    virtual ssize_t             write(void const * buf, size_t count);

private:
    bool                        define_address();

    tcp_bio_client::pointer_t   f_client = tcp_bio_client::pointer_t();
    sockaddr_storage            f_address = sockaddr_storage();
    socklen_t                   f_length = 0;
};



} // namespace ed
// vim: ts=4 sw=4 et
