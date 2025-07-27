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
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// self
//
#include    <eventdispatcher/connection.h>
#include    <eventdispatcher/tcp_bio_client.h>


// C
//
#include    <sys/socket.h>



namespace ed
{



class tcp_server_client_connection
    : public connection
{
public:
    typedef std::shared_ptr<tcp_server_client_connection>    pointer_t;

                                tcp_server_client_connection(tcp_bio_client::pointer_t client);
    virtual                     ~tcp_server_client_connection() override;

    void                        close();
    addr::addr const &          get_client_address();
    addr::addr const &          get_remote_address();

    // connection implementation
    virtual bool                is_reader() const override;
    virtual int                 get_socket() const override;

    // new callbacks
    virtual ssize_t             read(void * buf, size_t count);
    virtual ssize_t             write(void const * buf, size_t count);

private:
    tcp_bio_client::pointer_t   f_client = tcp_bio_client::pointer_t();
    addr::addr                  f_client_address = addr::addr();
    addr::addr                  f_remote_address = addr::addr();
};



} // namespace ed
// vim: ts=4 sw=4 et
