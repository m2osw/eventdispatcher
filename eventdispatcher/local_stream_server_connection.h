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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

/** \file
 * \brief Handle a Unix stream oriented connection.
 *
 * This class defines a server that will listen for local Unix stream
 * connections.
 *
 * You have to derive from this class and implement the process_access()
 * function. In that function, you call accept() and then create a
 * local_stream_server_client_connection (or a derive class, such as
 * the local_stream_server_client_message_connection which automatically
 * handles messages).
 */


// self
//
#include    <eventdispatcher/connection.h>
#include    <eventdispatcher/utils.h>


// libaddr
//
#include    <libaddr/addr_unix.h>



namespace ed
{



class local_stream_server_connection
    : public connection
{
public:
    typedef std::shared_ptr<local_stream_server_connection>     pointer_t;

                        local_stream_server_connection(
                                  addr::addr_unix const & address
                                , int max_connections = MAX_CONNECTIONS
                                , bool force_reuse_addr = false
                                , bool close_on_exec = true);
    virtual             ~local_stream_server_connection();

    addr::addr_unix     get_addr() const;
    int                 get_max_connections() const;
    snapdev::raii_fd_t  accept();
    bool                get_close_on_exec() const;
    void                set_close_on_exec(bool yes = true);

    // connection implementation
    //
    virtual bool        is_listener() const override;
    virtual int         get_socket() const override;

private:
    addr::addr_unix          f_address = addr::addr_unix();
    int                 f_max_connections = MAX_CONNECTIONS;
    snapdev::raii_fd_t  f_socket = snapdev::raii_fd_t();
    int                 f_accepted_socket = -1;
    bool                f_close_on_exec = false;
};



} // namespace ed
// vim: ts=4 sw=4 et
